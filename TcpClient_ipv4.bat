@echo off
::::::::::::::::::
::�����IP��ַ
set cmd="strIP=127.0.0.1"
::����˶˿�
set cmd=%cmd% nPort=4567
::�����߳�����
set cmd=%cmd% nThread=1
::�������ٸ��ͻ���
set cmd=%cmd% nClient=10
::::::�ȴ�socket��дʱ��ʵ�ʷ���
::ÿ���ͻ�����nSendSleep(����)ʱ����
::����д��nMsg��Login��Ϣ
::ÿ����Ϣ100�ֽڣ�Login��
set cmd=%cmd% nMsg=1
set cmd=%cmd% nSendSleep=10
::�ͻ��˷��ͻ�������С���ֽڣ�
set cmd=%cmd% nSendBuffSize=10240
::�ͻ��˽��ջ�������С���ֽڣ�
set cmd=%cmd% nRecvBuffSize=10240
::�����յ��ķ������ϢID�Ƿ�����
set cmd=%cmd% -checkMsgID
::���-���͵���Ϣ�ѱ���������Ӧ
::�յ���������Ӧ��ŷ�����һ����Ϣ
set cmd=%cmd% -chekSendBack
::::::
::::::
::�������� �������
TcpClient %cmd%

pause