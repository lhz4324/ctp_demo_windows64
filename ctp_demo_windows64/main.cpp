#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include "CustomTradeSpi.h"

#include"httplib.h"
//#include <windows.h> 

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"



using namespace std;

// 链接库
#pragma comment (lib, "thostmduserapi_se.lib")
#pragma comment (lib, "thosttraderapi_se.lib")

// ---- 全局变量 ---- //
// 公共参数
TThostFtdcBrokerIDType gBrokerID = "9999";                         // 模拟经纪商代码
TThostFtdcInvestorIDType gInvesterID;                         // 投资者账户名
TThostFtdcPasswordType gInvesterPassword;                     // 投资者密码


// 交易参数
CThostFtdcTraderApi* g_pTradeUserApi = nullptr;                    // 交易指针
//char gTradeFrontAddr[] = "tcp://180.168.146.187:10130";            // 模拟交易前置地址,根据时间段需要调整
//char gTradeFrontAddr[] = "tcp://180.168.146.187:10201";            // 模拟交易前置地址,根据时间段需要调整

char gTradeFrontAddr[]= "tcp://180.168.146.187:10130";

TThostFtdcInstrumentIDType g_pTradeInstrumentID = "IF2304";        // 所交易的合约代码
TThostFtdcDirectionType gTradeDirection = THOST_FTDC_D_Sell;       // 买卖方向
TThostFtdcPriceType gLimitPrice = 22735;                           // 交易价格


vector <string>vecInvesterID = {};  // 投资者账户名
vector <string>vecInvesterPassword = {};  // 投资者密码
vector <string>vecTradeFrontAddr = {};//投资者前置机
vector <int>vecContractAlertNum = {};//投资者ContractAlertNum
vector <int>vecAllContractAlertNum = {};//投资者AllContractAlertNum


string readFromJsonData(string msg) // msg为传入的json格式文件{"cmd":"getType","whichType":"type1","result":0}
{
	rapidjson::Document doc;
	doc.Parse(msg.c_str());
	if (!doc.IsObject())  // 判断是不是json格式的文件
	{
		return "is not a jsonfile";
	}
	if (!doc.HasMember("Alluser"))  // 判断键中是否存在"cmd"
	{
		//std::cout<<"is not contain cmd"<<std::endl;
		return "is not contain cmd";
	}


	rapidjson::Value& m = doc["Alluser"];
	if (m.IsArray()) {
		for (int i = 0; i < m.Size(); i++) {
			rapidjson::Value& id = m[i]["InvesterID"];
			rapidjson::Value& pw = m[i]["InvesterPassword"];
			rapidjson::Value& tf = m[i]["TradeFrontAddr"];
			rapidjson::Value& cn = m[i]["ContractAlertNum"];
			rapidjson::Value& an = m[i]["AllContractAlertNum"];
			vecInvesterID.push_back(id.GetString());
			vecInvesterPassword.push_back(pw.GetString());
			vecTradeFrontAddr.push_back(tf.GetString());
			
			vecContractAlertNum.push_back(atoi(cn.GetString()));
			vecAllContractAlertNum.push_back(atoi(an.GetString()));
		}
	}

	rapidjson::Value& jsonValue = doc["Alluser"][0]["InvesterID"]; // 获取值

	if (!jsonValue.IsString())  // 判断结果是不是字符串 ，toInt()判断是不是整数 ，依次类推
	{
		return "is not string type";
	}
	return  jsonValue.GetString();
}

string readfile(const char* filename) {
	//FILE* fp = fopen(filename, "rb");

	FILE* fp;
	errno_t err = fopen_s(&fp, filename, "rb");

	if (!fp) {
		printf("open failed! file: %s", filename);
		return "";
	}


	char buf[1024 * 16]; //新建缓存区
	string result;
	/*循环读取文件，直到文件读取完成*/
	while (int n = fgets(buf, 1024 * 16, fp) != NULL)
	{
		result.append(buf);
		//cout << buf << endl;
	}
	fclose(fp);
	return result;
}

//取标的首部函数：
//string getHead(const string& str)
//{
//	
//	char c;
//	c = str[1];//读取第二个字符。
//	if (c >= '0' && c <= '9')
//	{	
//		//printf("是数字\n");
//		return str.substr(0, 1);
//	}
//	else  if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
//	{
//		//printf("是字母\n");
//		return str.substr(0, 2);
//	}
//	
//	else
//	{
//		printf("是特殊字符\n");
//	}
//	
//
//}



int main()
{	


	std::string jsonstr = readfile(".\\userlist.json");//将文件转换为jsonstr字符串
	std::string msg = readFromJsonData(jsonstr);//将jsonstr转换至vecInvesterID，vecInvesterPassword
	int user_Length = vecInvesterID.size();

	vector<CThostFtdcTraderApi*> m_vecpTradeUserApi;
	vector<CustomTradeSpi*> m_vecTradeSpi;

	for (int i = 0; i < user_Length; i++) {
		strcpy(gInvesterID, vecInvesterID[i].c_str());
		strcpy(gInvesterPassword, vecInvesterPassword[i].c_str());
		strcpy(gTradeFrontAddr, vecTradeFrontAddr[i].c_str());

		//初始化交易线程
		std::cout << "初始化交易..." << endl;



		g_pTradeUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(); // 创建交易实例
		CustomTradeSpi* pTradeSpi = new CustomTradeSpi(vecInvesterID[i],vecContractAlertNum[i],vecAllContractAlertNum[i]);               // 创建交易回调实例

		
		g_pTradeUserApi->RegisterSpi(pTradeSpi);                      // 注册事件类
		g_pTradeUserApi->SubscribePublicTopic(THOST_TERT_RESTART);    // 订阅公共流
		g_pTradeUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);   // 订阅私有流
		g_pTradeUserApi->RegisterFront(gTradeFrontAddr);              // 设置交易前置地址
		g_pTradeUserApi->Init();                                      // 连接运行

		m_vecpTradeUserApi.push_back(g_pTradeUserApi);
		m_vecTradeSpi.push_back(pTradeSpi);

		//如果发生错误或执行完毕，进入下一个循环，否则一直运行
		while (!pTradeSpi->taskdone) {
			Sleep(10);
		}
	}



	
	
		//for (auto& item : m_vecTradeSpi[0]->m_ContractInstmap)
		//{
		//	cout <<"m_ContractInstmap key:"<<item.first << endl;
		//	for (auto& si : item.second)
		//	{
		//		cout << "m_ContractInstmap value instname: "<<si.first << endl;
		//		cout << "m_ContractInstmap value instcount: "<<si.second << endl;
		//	}
		//}

		//for (auto& item : m_vecTradeSpi[0]->m_ContractNummap)
		//{
		//	cout << "m_ContractNummap key:" << item.first << endl;
		//	cout << "m_ContractNummap value count: " << item.second << endl;
		//
		//}

		//cout << m_vecTradeSpi[0]->totalContractNum << endl;
	std::cout << "ALL task done" << endl;
	int i=getchar();

	return 0;
}

