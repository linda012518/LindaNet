@echo off
::::::::::::::::::
::�����IP��ַ
set cmd="strIP=any"
::����˶˿�
set cmd=%cmd% nPort=4567
::��Ϣ�����߳�����
set cmd=%cmd% nThread=1
::�ͻ�����������
set cmd=%cmd% nMaxClient=100064
::�ͻ��˷��ͻ�������С���ֽڣ�
set cmd=%cmd% nSendBuffSize=10240
::�ͻ��˽��ջ�������С���ֽڣ�
set cmd=%cmd% nRecvBuffSize=10240
::�յ���Ϣ�󽫷���Ӧ����Ϣ
set cmd=%cmd% -sendback
::��ʾ���ͻ�������д��
::������sendfull��ʾʱ����ʾ������Ϣ������
set cmd=%cmd% -sendfull
::�����յ��Ŀͻ�����ϢID�Ƿ�����
set cmd=%cmd% -checkMsgID
::�Զ����־ δʹ��
set cmd=%cmd% -p

::�������� �������
TcpServer %cmd%

pause