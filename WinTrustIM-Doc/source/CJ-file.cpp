//*******************************************************
//gloox一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
//*******************************************************

//*******************************************************
//文件传输，建议使用http方式
//在消息体中，表明为文件类型的消息，收到后，处理文件
//文件的发送与接收，都使用htpp
//*******************************************************

class MessageTest : RosterListener, MessageSessionHandler, ConnectionListener, MessageEventHandler, MessageHandler, ChatStateHandler, LogHandler
{
    public:
        MessageTest() : m_session( 0 ), m_messageEventFilter( 0 ), m_chatStateFilter( 0 ) {}

        virtual ~MessageTest() {}

        void start()
        {
            JID jid( "ca23@192.168.199.225/ca23" );
            j = new Client( jid, "123456" );
            j->registerConnectionListener( this ); //注册client的recv
            j->registerMessageSessionHandler( this, 0 ); //注册会话等类
            j->rosterManager()->registerRosterListener( this ); //注册花名册管理
            j->disco()->setVersion( "messageTest", gloox::GLOOX_VERSION, "Linux" ); //设置版本
            j->disco()->setIdentity( "client", "bot" ); //设置XMPP实体（一个XML概念）
            j->disco()->addFeature( gloox::XMLNS_CHAT_STATES );
            j->logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this ); //打开LOG
            j->setPresence( Presence::Chat, 1, "Play Music WoWo-Gala!" ); //设置自己的状态还有在签名
            
            if( j->connect( false ) )
            {
                ConnectionError ce = ConnNoError;
                while( ce == ConnNoError )
                {
                    ce = j->recv();
                }
                printf( "ce: %d\n", ce );
            }
            
            delete( j );
        }
        
        virtual void onConnect()
        {
            printf( "connected!!!\n" );
        }

        virtual void onDisconnect( ConnectionError e )
        {
            printf( "message_test: disconnected: %d\n", e );
            if( e == ConnAuthenticationFailed ) printf( "auth failed. reason: %d\n", j->authError() );
        }

        virtual bool onTLSConnect( const CertInfo& info )
        {
            time_t from( info.date_from );
            time_t to( info.date_to );
            printf( "status: %d\nissuer: %s\npeer: %s\nprotocol: %s\nmac: %s\ncipher: %s\ncompression: %s\n" "from: %s\nto: %s\n", info.status, info.issuer.c_str(), info.server.c_str(), info.protocol.c_str(), info.mac.c_str(), info.cipher.c_str(), info.compression.c_str(), ctime( &from ), ctime( &to ) );
            return true;
        }

        virtual void handleMessage( const Message & msg, MessageSession * session)
        {
            m_messageEventFilter->raiseMessageEvent( MessageEventDisplayed );
            m_messageEventFilter->raiseMessageEvent( MessageEventComposing );
            m_chatStateFilter->setChatState( ChatStateComposing );
            
            //这里的消息命令。都是自己定义的，用来测试的
            //因为是消息机制，所以，用消息代表一个命令，发过来触发流程操作
            if( msg.body() == "test-file-trans-command" )
            {
                //确认为文件消息后
                //开始使用http进行下载文件
            }
            else if( msg.body() == "test-voice-trans-command" )
            {

            }
            else
            {
                //message body消息体判断，这里判断的原因是 virtusl 实现 handleMessage 句柄后，所有的涉及到
                //xmpp message结构消息的内容都会流经此处，所以，需要过滤一下
                if( msg.subtype() == Message::Chat )
                {
                    if( msg.body().length() == 0 )
                    {
                        return;
                    }
                    else
                    {
                        printf( "type: %d, subject: %s, message: %s, thread id: %s, from: %s\n", msg.subtype(), msg.subject().c_str(), msg.body().c_str(), msg.thread().c_str(), msg.from().full().c_str() );
                    }
                }
            }
        }

        virtual void handleMessageEvent( const JID& from, MessageEventType event )
        {
            printf( "received event: %d from: %s\n", event, from.full().c_str() );
        }

        virtual void handleChatState( const JID& from, ChatStateType state )
        {
            printf( "received state: %d from: %s\n", state, from.full().c_str() );
        }

        virtual void handleMessageSession( MessageSession *session )
        {
            printf( "got new session\n");
            if( m_session ) j->disposeMessageSession( m_session );
            m_session = session;
            m_session->registerMessageHandler( this );
            m_messageEventFilter = new MessageEventFilter( m_session );
            m_messageEventFilter->registerMessageEventHandler( this );
            m_chatStateFilter = new ChatStateFilter( m_session );
            m_chatStateFilter->registerChatStateHandler( this );
        }
        
        //log输出事件，这个事件，在gloox流程中，很多地方都会触发，所以，log信息量会比较大
        virtual void handleLog( LogLevel level, LogArea area, const std::string& message )
        {
            printf("#-----------#  log: level: %d, area: %d, %s\n", level, area, message.c_str() );
        }
        
        virtual void onResourceBindError( ResourceBindError error )
        {
            printf( "onResourceBindError: %d\n", error );
        }

        virtual void onSessionCreateError( SessionCreateError error )
        {
            printf( "onSessionCreateError: %d\n", error );
        }

    private:
        Client * j;
        MessageSession * m_session;
        MessageEventFilter * m_messageEventFilter;
        ChatStateFilter * m_chatStateFilter;
        MucTest * mt;
};

int main( int argc, char* argv[] )
{
    //测试消息，包括聊天室，文件传输，语音消息，图片等，具体参考消息命令
    MessageTest * r = new MessageTest();
    r->start();
    delete( r );

    return 0;
}

