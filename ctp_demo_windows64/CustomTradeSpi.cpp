#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <time.h>
#include <thread>
#include <chrono>
#include "CustomTradeSpi.h"

#include <sstream>
#include<iomanip>
#include <iostream>
#include <fstream>
#include <string>

//#include <direct.h>



// ---- 全局参数声明 ---- //
extern TThostFtdcBrokerIDType gBrokerID;                      // 模拟经纪商代码
extern TThostFtdcInvestorIDType gInvesterID;                  // 投资者账户名
extern TThostFtdcPasswordType gInvesterPassword;              // 投资者密码
extern CThostFtdcTraderApi* g_pTradeUserApi;                  // 交易指针
extern char gTradeFrontAddr[];                                // 模拟交易前置地址
extern TThostFtdcInstrumentIDType g_pTradeInstrumentID;       // 所交易的合约代码
extern TThostFtdcDirectionType gTradeDirection;               // 买卖方向
extern TThostFtdcPriceType gLimitPrice;                       // 交易价格

// 会话参数
TThostFtdcFrontIDType	trade_front_id;	//前置编号
TThostFtdcSessionIDType	session_id;	//会话编号
TThostFtdcOrderRefType	order_ref;	//报单引用
time_t lOrderTime;
time_t lOrderOkTime;



CustomTradeSpi::CustomTradeSpi(const std::string& username,int contractAlertNum, int allContractAlertNum)
{	
	//struct tm* tm;//赋值结构体名称 tm
	//time_t t;//得到从1970年1月1日到目前的秒数 t
	//t = time(NULL);
	//tm = localtime(&t);


	//std::string min = std::to_string(tm->tm_min);
	//std::string sec = std::to_string(tm->tm_sec);


	//std::string mylog = "account_"+min + sec;

	this->accountname = username;

	this->ContractAlertNum = contractAlertNum;
	this->AllContractAlertNum = allContractAlertNum;
	std::string mylog = "account_" + username;
	this->m_logger= spdlog::basic_logger_mt(mylog, "log.txt");
}

void CustomTradeSpi::OnFrontConnected()
{
	std::unique_lock<std::mutex> unique(mt);
	std::cout << "=====建立网络连接成功=====" << std::endl;

	m_logger->info("=====建立网络连接成功=====");
	m_logger->flush();
	// 开始登录
	unique.unlock();
	reqUserLogin();
}

void CustomTradeSpi::OnRspUserLogin(
	CThostFtdcRspUserLoginField* pRspUserLogin,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{

	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << "=====账户登录成功=====" << std::endl;
		loginFlag = true;
		std::cout << "交易日： " << pRspUserLogin->TradingDay << std::endl;
		std::cout << "登录时间： " << pRspUserLogin->LoginTime << std::endl;
		std::cout << "经纪商： " << pRspUserLogin->BrokerID << std::endl;
		std::cout << "帐户名： " << pRspUserLogin->UserID << std::endl;
		std::cout << "------------------------------------------------" << std::endl;


		m_logger->info("账户登录成功！ 交易日：{},登录时间：{},经纪商：{},帐户名：{}",
			pRspUserLogin->TradingDay, pRspUserLogin->LoginTime, pRspUserLogin->BrokerID, pRspUserLogin->UserID);

		// 保存会话参数
		trade_front_id = pRspUserLogin->FrontID;
		session_id = pRspUserLogin->SessionID;
		strcpy(order_ref, pRspUserLogin->MaxOrderRef);

		unique.unlock();

		// 投资者结算结果确认
		//reqSettlementInfoConfirm();

		//查询报单
		reqQryOrder();

	}
	else {
		std::cout << "=====账户登录失败=====" << std::endl;
		m_logger->error("账户登录失败");
	}

	m_logger->flush();

}

void CustomTradeSpi::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	isErrorRspInfo(pRspInfo);
}

void CustomTradeSpi::OnFrontDisconnected(int nReason)
{
	m_logger->error("网络连接断开,错误码：{}", nReason);
	m_logger->flush();
}

void CustomTradeSpi::OnHeartBeatWarning(int nTimeLapse)
{
	m_logger->error("网络心跳超时,距上次连接时间：{}", nTimeLapse);
	m_logger->flush();
}

void CustomTradeSpi::OnRspUserLogout(
	CThostFtdcUserLogoutField* pUserLogout,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		loginFlag = false; // 登出就不能再交易了 
		//std::cout << "=====账户登出成功=====" << std::endl;
		//std::cout << "经纪商： " << pUserLogout->BrokerID << std::endl;
		//std::cout << "帐户名： " << pUserLogout->UserID << std::endl;

		m_logger->info("账户登出成功,经纪商：{},帐户名：{}", pUserLogout->BrokerID, pUserLogout->UserID);
		m_logger->flush();
	}
}

void CustomTradeSpi::OnRspSettlementInfoConfirm(
	CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		//std::cout << "=====投资者结算结果确认成功=====" << std::endl;
		//std::cout << "确认日期： " << pSettlementInfoConfirm->ConfirmDate << std::endl;
		//std::cout << "确认时间： " << pSettlementInfoConfirm->ConfirmTime << std::endl;

		unique.unlock();

		// 请求查询合约
		//reqQueryInstrument();

	}

}

void CustomTradeSpi::OnRspQryInstrument(
	CThostFtdcInstrumentField* pInstrument,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		//std::cout << "=====查询合约结果成功=====" << std::endl;
		//std::cout << "交易所代码： " << pInstrument->ExchangeID << std::endl;
		//std::cout << "合约代码： " << pInstrument->InstrumentID << std::endl;
		//std::cout << "合约在交易所的代码： " << pInstrument->ExchangeInstID << std::endl;
		//std::cout << "执行价： " << pInstrument->StrikePrice << std::endl;
		//std::cout << "到期日： " << pInstrument->EndDelivDate << std::endl;
		//std::cout << "当前交易状态： " << pInstrument->IsTrading << std::endl;

		unique.unlock();
		// 请求查询投资者资金账户
		//reqQueryTradingAccount();
	}
}

void CustomTradeSpi::OnRspQryTradingAccount(
	CThostFtdcTradingAccountField* pTradingAccount,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		//std::cout << "=====查询投资者资金账户成功=====" << std::endl;
		//std::cout << "投资者账号： " << pTradingAccount->AccountID << std::endl;
		//std::cout << "可用资金： " << pTradingAccount->Available << std::endl;
		//std::cout << "可取资金： " << pTradingAccount->WithdrawQuota << std::endl;
		//std::cout << "当前保证金: " << pTradingAccount->CurrMargin << std::endl;
		//std::cout << "平仓盈亏： " << pTradingAccount->CloseProfit << std::endl;

		unique.unlock();
		// 请求查询投资者持仓
		//reqQueryInvestorPosition();
	}
}

void CustomTradeSpi::OnRspQryInvestorPosition(
	CThostFtdcInvestorPositionField* pInvestorPosition,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		//std::cout << "=====查询投资者持仓成功=====" << std::endl;
		if (pInvestorPosition)
		{
			//std::cout << "合约代码： " << pInvestorPosition->InstrumentID << std::endl;
			//std::cout << "开仓价格： " << pInvestorPosition->OpenAmount << std::endl;
			//std::cout << "开仓量： " << pInvestorPosition->OpenVolume << std::endl;
			//std::cout << "开仓方向： " << pInvestorPosition->PosiDirection << std::endl;
			//std::cout << "占用保证金：" << pInvestorPosition->UseMargin << std::endl;
		}
		else
			//std::cout << "----->该合约未持仓" << std::endl;

			unique.unlock();


		// 报单录入请求（这里是一部接口，此处是按顺序执行）
		if (loginFlag)
		{
			//reqOrderInsert();
			//reqQryOrder();
		}


		//if (loginFlag)
		//	reqOrderInsertWithParams(g_pTradeInstrumentID, gLimitPrice, 1, gTradeDirection); // 自定义一笔交易

		// 策略交易
		//std::cout << "=====开始进入策略交易=====" << std::endl;
		//while (loginFlag)
		//	StrategyCheckAndTrade(g_pTradeInstrumentID, this);
	}
}


void CustomTradeSpi::OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo)&& pOrder != nullptr)
	{
		std::unique_lock<std::mutex> unique(mt);
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << "=====请求查询报单成功=====" << std::endl;
		std::cout << "合约代码： " << pOrder->InstrumentID << std::endl;
		std::cout << "价格： " << pOrder->LimitPrice << std::endl;
		std::cout << "数量： " << pOrder->VolumeTotalOriginal << std::endl;
		std::cout << "开仓方向： " << pOrder->Direction << std::endl;
		std::cout << "------------------------------------------------" << std::endl;

		m_logger->info("请求查询报单成功!合约代码：{},价格：{},数量：{},开仓方向：{}", pOrder->InstrumentID, pOrder->LimitPrice,
			pOrder->VolumeTotalOriginal, pOrder->Direction);
		m_logger->flush();
		this->firsttime_tocsv++;
		qryOrder_to_csv(pOrder);

	}
	else {
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << " =====请求查询报单失败=====" << std::endl;
		std::cout << "------------------------------------------------" << std::endl;

		m_logger->info("请求查询报单失败");
		m_logger->flush();
	}

	if (bIsLast == true) {
		this->taskdone = true;
	}
}


void CustomTradeSpi::OnRspOrderInsert(
	CThostFtdcInputOrderField* pInputOrder,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		std::cout << "=====报单录入成功=====" << std::endl;
		std::cout << "合约代码： " << pInputOrder->InstrumentID << std::endl;
		std::cout << "价格： " << pInputOrder->LimitPrice << std::endl;
		std::cout << "数量： " << pInputOrder->VolumeTotalOriginal << std::endl;
		std::cout << "开仓方向： " << pInputOrder->Direction << std::endl;
	}
}

void CustomTradeSpi::OnRspOrderAction(
	CThostFtdcInputOrderActionField* pInputOrderAction,
	CThostFtdcRspInfoField* pRspInfo,
	int nRequestID,
	bool bIsLast)
{
	if (!isErrorRspInfo(pRspInfo))
	{
		std::unique_lock<std::mutex> unique(mt);
		std::cout << "=====报单操作成功=====" << std::endl;
		std::cout << "合约代码： " << pInputOrderAction->InstrumentID << std::endl;
		std::cout << "操作标志： " << pInputOrderAction->ActionFlag;
	}
}

void CustomTradeSpi::OnRtnOrder(CThostFtdcOrderField* pOrder)
{
	std::unique_lock<std::mutex> unique(mt);
	char str[10];
	sprintf(str, "%d", pOrder->OrderSubmitStatus);
	int orderState = atoi(str) - 48;	//报单状态0=已经提交，3=已经接受

	//std::cout << "=====收到报单应答=====" << std::endl;

	if (isMyOrder(pOrder))
	{
		if (isTradingOrder(pOrder))
		{
			std::cout << "--->>> 等待成交中！" << std::endl;
			//reqOrderAction(pOrder); // 这里可以撤单
			//reqUserLogout(); // 登出测试
		}
		else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
			std::cout << "--->>> 撤单成功！" << std::endl;
	}
}

void CustomTradeSpi::OnRtnTrade(CThostFtdcTradeField* pTrade)
{
	std::unique_lock<std::mutex> unique(mt);
	std::cout << "=====报单成功成交=====" << std::endl;
	std::cout << "成交时间： " << pTrade->TradeTime << std::endl;
	std::cout << "合约代码： " << pTrade->InstrumentID << std::endl;
	std::cout << "成交价格： " << pTrade->Price << std::endl;
	std::cout << "成交量： " << pTrade->Volume << std::endl;
	std::cout << "开平仓方向： " << pTrade->Direction << std::endl;
}

bool CustomTradeSpi::isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo)
{
	std::unique_lock<std::mutex> unique(mt);
	bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
	if (bResult) {
		std::cout << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
		this->taskdone = true;

		m_logger->error("返回错误,ErrorID={},ErrorMsg={}", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		m_logger->flush();
	}

	return bResult;
}

void CustomTradeSpi::reqUserLogin()
{
	std::unique_lock<std::mutex> unique(mt);

	CThostFtdcReqUserLoginField loginReq;
	memset(&loginReq, 0, sizeof(loginReq));
	strcpy(loginReq.BrokerID, gBrokerID);
	strcpy(loginReq.UserID, gInvesterID);
	strcpy(loginReq.Password, gInvesterPassword);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqUserLogin(&loginReq, requestID);
	if (!rt)
	{
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << ">>>>>>发送登录请求成功" << std::endl;
		std::cout << "------------------------------------------------" << std::endl;

		m_logger->info("发送登录请求成功");
		m_logger->flush();

	}
	else
	{
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << "--->>>发送登录请求失败" << std::endl;
		std::cout << "------------------------------------------------" << std::endl;

		m_logger->error("发送登录请求失败");
		m_logger->flush();
	}


}

void CustomTradeSpi::reqUserLogout()
{
	CThostFtdcUserLogoutField logoutReq;
	memset(&logoutReq, 0, sizeof(logoutReq));
	strcpy(logoutReq.BrokerID, gBrokerID);
	strcpy(logoutReq.UserID, gInvesterID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqUserLogout(&logoutReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送登出请求成功" << std::endl;
	else
		std::cerr << "--->>>发送登出请求失败" << std::endl;
}


void CustomTradeSpi::reqSettlementInfoConfirm()
{
	std::unique_lock<std::mutex> unique(mt);
	CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
	memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
	strcpy(settlementConfirmReq.BrokerID, gBrokerID);
	strcpy(settlementConfirmReq.InvestorID, gInvesterID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqSettlementInfoConfirm(&settlementConfirmReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者结算结果确认请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者结算结果确认请求失败" << std::endl;
}

void CustomTradeSpi::reqQueryInstrument()
{
	std::unique_lock<std::mutex> unique(mt);
	CThostFtdcQryInstrumentField instrumentReq;
	memset(&instrumentReq, 0, sizeof(instrumentReq));
	strcpy(instrumentReq.InstrumentID, g_pTradeInstrumentID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqQryInstrument(&instrumentReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送合约查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送合约查询请求失败" << std::endl;
}

void CustomTradeSpi::reqQueryTradingAccount()
{
	std::unique_lock<std::mutex> unique(mt);
	CThostFtdcQryTradingAccountField tradingAccountReq;
	memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
	strcpy(tradingAccountReq.BrokerID, gBrokerID);
	strcpy(tradingAccountReq.InvestorID, gInvesterID);
	static int requestID = 0; // 请求编号
	std::this_thread::sleep_for(std::chrono::milliseconds(1700)); // 有时候需要停顿一会才能查询成功

	int rt = g_pTradeUserApi->ReqQryTradingAccount(&tradingAccountReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者资金账户查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者资金账户查询请求失败" << std::endl;
}

void CustomTradeSpi::reqQueryInvestorPosition()
{
	std::unique_lock<std::mutex> unique(mt);
	CThostFtdcQryInvestorPositionField postionReq;
	memset(&postionReq, 0, sizeof(postionReq));
	strcpy(postionReq.BrokerID, gBrokerID);
	strcpy(postionReq.InvestorID, gInvesterID);
	strcpy(postionReq.InstrumentID, g_pTradeInstrumentID);
	static int requestID = 0; // 请求编号

	std::this_thread::sleep_for(std::chrono::milliseconds(1700)); // 有时候需要停顿一会才能查询成功

	int rt = g_pTradeUserApi->ReqQryInvestorPosition(&postionReq, requestID);
	if (!rt)
		std::cout << ">>>>>>发送投资者持仓查询请求成功" << std::endl;
	else
		std::cerr << "--->>>发送投资者持仓查询请求失败" << std::endl;
}

void CustomTradeSpi::reqQryOrder() // 请求查询报单
{
	std::unique_lock<std::mutex> unique(mt);
	CThostFtdcQryOrderField pQryOrder;
	memset(&pQryOrder, 0, sizeof(pQryOrder));
	strcpy(pQryOrder.BrokerID, gBrokerID);
	strcpy(pQryOrder.InvestorID, gInvesterID);
	//strcpy(pQryOrder.InstrumentID, g_pTradeInstrumentID);
	static int requestID = 0; // 请求编号

	std::this_thread::sleep_for(std::chrono::milliseconds(1700)); // 有时候需要停顿一会才能查询成功

	int rt = g_pTradeUserApi->ReqQryOrder(&pQryOrder, requestID);
	if (!rt)
	{
		std::cout << "------------------------------------------------" << std::endl;
		std::cout << ">>>>>>发送请求查询报单成功" << std::endl;
		std::cout << "------------------------------------------------" << std::endl;

		m_logger->info("发送请求查询报单成功");
		m_logger->flush();
	}

	else
	{
		std::cout << "------------------------------------------------" << std::endl;
		std::cerr << "--->>>发送请求查询报单失败" << std::endl;
		std::cout << "------------------------------------------------" << std::endl;

		m_logger->info("发送请求查询报单失败");
		m_logger->flush();
	}

}
void CustomTradeSpi::reqOrderInsert()
{
	std::unique_lock<std::mutex> unique(mt);
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///经纪公司代码
	strcpy(orderInsertReq.BrokerID, gBrokerID);
	///投资者代码
	strcpy(orderInsertReq.InvestorID, gInvesterID);
	///合约代码
	strcpy(orderInsertReq.InstrumentID, g_pTradeInstrumentID);
	///报单引用
	strcpy(orderInsertReq.OrderRef, order_ref);
	///报单价格条件: 限价
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///买卖方向: 
	//orderInsertReq.Direction = gTradeDirection;
	orderInsertReq.Direction = THOST_FTDC_D_Buy;
	///组合开平标志: 开仓
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///组合投机套保标志
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	orderInsertReq.LimitPrice = gLimitPrice;
	///数量：1
	orderInsertReq.VolumeTotalOriginal = 1;
	///有效期类型: 当日有效
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///成交量类型: 任何数量
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	orderInsertReq.MinVolume = 1;
	///触发条件: 立即
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///强平原因: 非强平
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	orderInsertReq.IsAutoSuspend = 0;
	///用户强评标志: 否
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单录入请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单录入请求失败" << std::endl;
}

void CustomTradeSpi::reqOrderInsert(
	TThostFtdcInstrumentIDType instrumentID,
	TThostFtdcPriceType price,
	TThostFtdcVolumeType volume,
	TThostFtdcDirectionType direction)
{
	CThostFtdcInputOrderField orderInsertReq;
	memset(&orderInsertReq, 0, sizeof(orderInsertReq));
	///经纪公司代码
	strcpy(orderInsertReq.BrokerID, gBrokerID);
	///投资者代码
	strcpy(orderInsertReq.InvestorID, gInvesterID);
	///合约代码
	strcpy(orderInsertReq.InstrumentID, instrumentID);
	///报单引用
	strcpy(orderInsertReq.OrderRef, order_ref);
	///报单价格条件: 限价
	orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///买卖方向: 
	orderInsertReq.Direction = direction;
	///组合开平标志: 开仓
	orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///组合投机套保标志
	orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	///价格
	orderInsertReq.LimitPrice = price;
	///数量：1
	orderInsertReq.VolumeTotalOriginal = volume;
	///有效期类型: 当日有效
	orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
	///成交量类型: 任何数量
	orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	orderInsertReq.MinVolume = 1;
	///触发条件: 立即
	orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
	///强平原因: 非强平
	orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	orderInsertReq.IsAutoSuspend = 0;
	///用户强评标志: 否
	orderInsertReq.UserForceClose = 0;

	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单录入请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单录入请求失败" << std::endl;
}

void CustomTradeSpi::reqOrderAction(CThostFtdcOrderField* pOrder)
{
	static bool orderActionSentFlag = false; // 是否发送了报单
	if (orderActionSentFlag)
		return;

	CThostFtdcInputOrderActionField orderActionReq;
	memset(&orderActionReq, 0, sizeof(orderActionReq));
	///经纪公司代码
	strcpy(orderActionReq.BrokerID, pOrder->BrokerID);
	///投资者代码
	strcpy(orderActionReq.InvestorID, pOrder->InvestorID);
	///报单操作引用
	//	TThostFtdcOrderActionRefType	OrderActionRef;
	///报单引用
	strcpy(orderActionReq.OrderRef, pOrder->OrderRef);
	///请求编号
	//	TThostFtdcRequestIDType	RequestID;
	///前置编号
	orderActionReq.FrontID = trade_front_id;
	///会话编号
	orderActionReq.SessionID = session_id;
	///交易所代码
	//	TThostFtdcExchangeIDType	ExchangeID;
	///报单编号
	//	TThostFtdcOrderSysIDType	OrderSysID;
	///操作标志
	orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
	///价格
	//	TThostFtdcPriceType	LimitPrice;
	///数量变化
	//	TThostFtdcVolumeType	VolumeChange;
	///用户代码
	//	TThostFtdcUserIDType	UserID;
	///合约代码
	strcpy(orderActionReq.InstrumentID, pOrder->InstrumentID);
	static int requestID = 0; // 请求编号
	int rt = g_pTradeUserApi->ReqOrderAction(&orderActionReq, ++requestID);
	if (!rt)
		std::cout << ">>>>>>发送报单操作请求成功" << std::endl;
	else
		std::cerr << "--->>>发送报单操作请求失败" << std::endl;
	orderActionSentFlag = true;
}

bool CustomTradeSpi::isMyOrder(CThostFtdcOrderField* pOrder)
{
	return ((pOrder->FrontID == trade_front_id) &&
		(pOrder->SessionID == session_id) &&
		(strcmp(pOrder->OrderRef, order_ref) == 0));
}

bool CustomTradeSpi::isTradingOrder(CThostFtdcOrderField* pOrder)
{
	return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
		(pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

//查询报单结果录入至csv文件

bool CustomTradeSpi::isFileExists_ifstream(std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}
void CustomTradeSpi::qryOrder_to_csv(CThostFtdcOrderField* pOrder)
{


	struct tm* tm;//赋值结构体名称 tm
	time_t t;//得到从1970年1月1日到目前的秒数 t
	t = time(NULL);
	tm = localtime(&t);

	std::string year = std::to_string(tm->tm_year + 1900);
	//std::string mon = std::to_string(tm->tm_mon + 1);
	std::string day = std::to_string(tm->tm_mday);
	std::stringstream ss;
	ss << std::setw(2) << std::setfill('0') << tm->tm_mon + 1;
	std::string mon = ss.str();

	std::string mydir = year + mon + day;

	int i = _mkdir(mydir.c_str());//创建目录,如果没有则创建




	std::ofstream outFile;
	std::string filename = gInvesterID;
	filename += ".csv";
	filename = mydir + "\\" + filename;

	if (isFileExists_ifstream(filename)&&this->firsttime_tocsv==1)
	{
		remove(filename.c_str());//如果文件存在则删除
	}

	outFile.open(filename, std::ios::app);


	// 写入标题行
	if (this->firsttime_tocsv == 1) {

		outFile << "BrokerID" << ','
			<< "InvestorID" << ','
			<< "InstrumentID" << ','
			<< "InstrumentID" << ','
			<< "OrderRef" << ','
			<< "UserID" << ','
			<< "OrderPriceType" << ','
			<< "Direction" << ','
			<< "CombOffsetFlag" << ','
			<< "CombHedgeFlag" << ','
			<< "LimitPrice" << ','
			<< "VolumeTotalOriginal" << ','
			<< "TimeCondition" << ','
			<< "GTDDate" << ','
			<< "VolumeCondition" << ','
			<< "MinVolume" << ','
			<< "ContingentCondition" << ','
			<< "StopPrice" << ','
			<< "ForceCloseReason" << ','
			<< "IsAutoSuspend" << ','
			<< "BusinessUnit" << ','
			<< "RequestID" << ','
			<< "OrderLocalID" << ','
			<< "ExchangeID" << ','
			<< "ParticipantID" << ','
			<< "ClientID" << ','
			<< "ExchangeInstID" << ','
			<< "TraderID" << ','
			<< "InstallID" << ','
			<< "OrderSubmitStatus" << ','
			<< "NotifySequence" << ','
			<< "TradingDay" << ','
			<< "SettlementID" << ','
			<< "OrderSysID" << ','
			<< "OrderSource" << ','
			<< "OrderStatus" << ','
			<< "OrderType" << ','
			<< "VolumeTraded" << ','
			<< "VolumeTotal" << ','
			<< "InsertDate" << ','
			<< "InsertTime" << ','
			<< "ActiveTime" << ','
			<< "SuspendTime" << ','
			<< "UpdateTime" << ','
			<< "CancelTime" << ','
			<< "ActiveTraderID" << ','
			<< "ClearingPartID" << ','
			<< "SequenceNo" << ','
			<< "FrontID" << ','
			<< "SessionID" << ','
			<< "UserProductInfo" << ','
			<< "StatusMsg" << ','
			<< "UserForceClose" << ','
			<< "ActiveUserID" << ','
			<< "BrokerOrderSeq" << ','
			<< "RelativeOrderSysID" << ','
			<< "ZCETotalTradedVolume" << ','
			<< "IsSwapOrder" << ','
			<< "BranchID" << ','
			<< "InvestUnitID" << ','
			<< "AccountID" << ','
			<< "CurrencyID" << ','
			<< "IPAddress" << ','
			<< "MacAddress" << std::endl;
	}

	
	
	//
	//// ********写入数据*********
	// 写入字符串(数字)
	outFile << pOrder->BrokerID << ','
		<< pOrder->InvestorID << ','
		<< pOrder->InstrumentID << ','
		<< pOrder->InstrumentID << ','
		<< pOrder->OrderRef << ','
		<< pOrder->UserID << ','
		<< pOrder->OrderPriceType << ','
		<< pOrder->Direction << ','
		<< pOrder->CombOffsetFlag << ','
		<< pOrder->CombHedgeFlag << ','
		<< pOrder->LimitPrice << ','
		<< pOrder->VolumeTotalOriginal << ','
		<< pOrder->TimeCondition << ','
		<< pOrder->GTDDate << ','
		<< pOrder->VolumeCondition << ','
		<< pOrder->MinVolume << ','
		<< pOrder->ContingentCondition << ','
		<< pOrder->StopPrice << ','
		<< pOrder->ForceCloseReason << ','
		<< pOrder->IsAutoSuspend << ','
		<< pOrder->BusinessUnit << ','
		<< pOrder->RequestID << ','
		<< pOrder->OrderLocalID << ','
		<< pOrder->ExchangeID << ','
		<< pOrder->ParticipantID << ','
		<< pOrder->ClientID << ','
		<< pOrder->ExchangeInstID << ','
		<< pOrder->TraderID << ','
		<< pOrder->InstallID << ','
		<< pOrder->OrderSubmitStatus << ','
		<< pOrder->NotifySequence << ','
		<< pOrder->TradingDay << ','
		<< pOrder->SettlementID << ','
		<< pOrder->OrderSysID << ','
		<< pOrder->OrderSource << ','
		<< pOrder->OrderStatus << ','
		<< pOrder->OrderType << ','
		<< pOrder->VolumeTraded << ','
		<< pOrder->VolumeTotal << ','
		<< pOrder->InsertDate << ','
		<< pOrder->InsertTime << ','
		<< pOrder->ActiveTime << ','
		<< pOrder->SuspendTime << ','
		<< pOrder->UpdateTime << ','
		<< pOrder->CancelTime << ','
		<< pOrder->ActiveTraderID << ','
		<< pOrder->ClearingPartID << ','
		<< pOrder->SequenceNo << ','
		<< pOrder->FrontID << ','
		<< pOrder->SessionID << ','
		<< pOrder->UserProductInfo << ','
		<< pOrder->StatusMsg << ','
		<< pOrder->UserForceClose << ','
		<< pOrder->ActiveUserID << ','
		<< pOrder->BrokerOrderSeq << ','
		<< pOrder->RelativeOrderSysID << ','
		<< pOrder->ZCETotalTradedVolume << ','
		<< pOrder->IsSwapOrder << ','
		<< pOrder->BranchID << ','
		<< pOrder->InvestUnitID << ','
		<< pOrder->AccountID << ','
		<< pOrder->CurrencyID << ','
		<< pOrder->IPAddress << ','
		<< pOrder->MacAddress << std::endl;


	outFile.close();
	
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << "Write to " << filename << ".csv is done!!!" << std::endl;
	std::cout << "------------------------------------------------" << std::endl;


	alerttoWechat(pOrder, this->ContractAlertNum, this->AllContractAlertNum);

	m_logger->info("Write to {}.csv",filename);
	m_logger->flush();


}

std::string CustomTradeSpi::getHead(const std::string& str)
{
	char c;
	c = str[1];//读取第二个字符。
	if (c >= '0' && c <= '9')
	{
		//printf("是数字\n");
		return str.substr(0, 1);
	}
	else  if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
	{
		//printf("是字母\n");
		return str.substr(0, 2);
	}

	else
	{
		printf("是特殊字符\n");
	}
}

void CustomTradeSpi::alerttoWechat(CThostFtdcOrderField* pOrder,int smallAlertNum,int totalAlertNum)
{
	std::string contract = getHead(pOrder->InstrumentID);
	int status = pOrder->OrderStatus;
	//std::cout << "porder de orderstatus"<<status << std::endl;

	if (status == 53)//已撤单
	{

		m_ContractInstmap[contract].push_back(std::make_pair(pOrder->InstrumentID, 2));
		m_ContractNummap[contract] += 2;

		if ((m_ContractNummap[contract] % smallAlertNum == 0) || ((m_ContractNummap[contract] - 1) % smallAlertNum == 0))
		{	//baojing
			m_logger->info("contract:{} has achieved {},(order canceled)", contract, m_ContractNummap[contract]);

		}

		totalContractNum += 2;
		if ((totalContractNum % totalAlertNum == 0)|| ((totalContractNum-1)%totalAlertNum==0))
		{	//baojing
			m_logger->info("yichedan totalContract has achieved {},(order canceled)", totalContractNum);

		}
	}
	else//其余的
	{
		m_ContractInstmap[contract].push_back(std::make_pair(pOrder->InstrumentID, 1));
		m_ContractNummap[contract] += 1;

		if (m_ContractNummap[contract] % smallAlertNum == 0)
		{	//baojing
			m_logger->info("contract:{} has achieved {}", contract, m_ContractNummap[contract]);

		}

		totalContractNum += 1;
		if (totalContractNum % totalAlertNum == 0)
		{	//baojing
			m_logger->info("totalContract has achieved {}", totalContractNum);

		}
	}
}
