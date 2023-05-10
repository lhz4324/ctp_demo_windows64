#pragma once
// ---- 派生的交易类 ---- //
#include "CTP_API/ThostFtdcTraderApi.h"
#include <mutex>
#include<unordered_map>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

class CustomTradeSpi : public CThostFtdcTraderSpi
{
	// ---- ctp_api部分回调接口 ---- //
public:

	CustomTradeSpi(const std::string& username,int ContractAlertNum,int AllContractAlertNum);

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	void OnFrontConnected();

	///登录请求响应
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	void OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	void OnHeartBeatWarning(int nTimeLapse);

	///登出请求响应
	void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///投资者结算结果确认响应
	void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询合约响应
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询资金账户响应
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///请求查询投资者持仓响应
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单录入请求响应
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单操作请求响应
	void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	///报单通知
	void OnRtnOrder(CThostFtdcOrderField* pOrder);

	///成交通知
	void OnRtnTrade(CThostFtdcTradeField* pTrade);

	//请求查询报单响应
	void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

	// ---- 自定义函数 ---- //
public:
	bool loginFlag; // 登陆成功的标识
	void reqOrderInsert(
		TThostFtdcInstrumentIDType instrumentID,
		TThostFtdcPriceType price,
		TThostFtdcVolumeType volume,
		TThostFtdcDirectionType direction); // 个性化报单录入，外部调用
private:
	void reqUserLogin(); // 登录请求
	void reqUserLogout(); // 登出请求
	void reqSettlementInfoConfirm(); // 投资者结果确认
	void reqQueryInstrument(); // 请求查询合约
	void reqQueryTradingAccount(); // 请求查询资金帐户
	void reqQueryInvestorPosition(); // 请求查询投资者持仓
	void reqOrderInsert(); // 请求报单录入

	void reqOrderAction(CThostFtdcOrderField* pOrder); // 请求报单操作
	bool isErrorRspInfo(CThostFtdcRspInfoField* pRspInfo); // 是否收到错误信息
	bool isMyOrder(CThostFtdcOrderField* pOrder); // 是否我的报单回报
	bool isTradingOrder(CThostFtdcOrderField* pOrder); // 是否正在交易的报单

	void qryOrder_to_csv(CThostFtdcOrderField* pOrder);//查询报单结果录入至csv文件
	bool isFileExists_ifstream(std::string& name);//判断文件是否存在
	std::string getHead(const std::string& str);//获取某标的的首部，即合约代码

	void reqQryOrder(); // 请求查询报单

	void alerttoWechat(CThostFtdcOrderField* pOrder, int smallAlertNum, int totalAlertNum);//向企业微信报警
	std::mutex mt;
public:
	int ContractAlertNum = 0;
	int AllContractAlertNum = 0;
	std::string accountname;
	bool taskdone = false;
	int firsttime_tocsv = 0;

	int totalContractNum = 0;
	std::shared_ptr<spdlog::logger> m_logger;

	std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> m_ContractInstmap;//合约，标的map
	std::unordered_map<std::string, int> m_ContractNummap;//合约数量map
};
