/**
 *  @file   TcpServer.h
 *  @brief   tcpͨѶ������
 *  @author   stanjiang
 *  @date   2012-03-29
*/
#ifndef _ZONE_TCPSERVER_H_
#define _ZONE_TCPSERVER_H_


// ����������ģʽ
enum ENMServerStartModel
{
    SERVER_START_NODAEMON = 0,
    SERVER_START_DAEMON = 1,
    SERVER_START_INVALID    
};

class CTcpServer
{
public:
    ~CTcpServer(){}

    static CTcpServer& Instance(void)
    {
        static CTcpServer s_inst;
        return s_inst;
    }

    /***
     *  @brief   ��ʼ��tcpsver
     *  @param   eModel: ����������ģʽ
     *  @return   0: ok , -1: error
     ***/
    int Init(ENMServerStartModel eModel);

private:
    /***
     *  @brief   ��ʼ��tcpsverΪ��̨����
     *  @param   eModel: ����������ģʽ
     *  @return   0: ok , -1: error
     ***/
    int InitDaemon(ENMServerStartModel eModel);

    static void sigusr1_handle(int iSigVal);
    static void sigusr2_handle(int iSigVal);
    
private:
    CTcpServer(){}
    CTcpServer(const CTcpServer&);
    CTcpServer& operator=(const CTcpServer&);
    

};


#endif

