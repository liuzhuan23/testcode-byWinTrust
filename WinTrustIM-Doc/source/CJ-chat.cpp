#include "gloox/gloox.h"
#include "gloox/client.h"
#include "gloox/connectionlistener.h"
#include "gloox/connectiontls.h"
#include "gloox/connectiontlsserver.h"
#include "gloox/connectiontcpclient.h"
#include "gloox/disco.h"
#include "gloox/discohandler.h"
#include "gloox/stanza.h"
#include "gloox/lastactivity.h"
#include "gloox/registration.h"
#include "gloox/logsink.h"
#include "gloox/loghandler.h"

//这部分头文件是关于聊天和注册的
#include "gloox/message.h"
#include "gloox/messagehandler.h"
#include "gloox/messagesessionhandler.h"
#include "gloox/messageeventhandler.h"
#include "gloox/messageeventfilter.h"
#include "gloox/chatstatehandler.h"
#include "gloox/chatstatefilter.h"
#include "gloox/connectiontcpclient.h"
#include "gloox/connectionsocks5proxy.h"
#include "gloox/connectionhttpproxy.h"

//这部分头文件是关于花名册和设置状态，签名（上线，下线，离开等）
#include "gloox/rostermanager.h"
#include "gloox/presence.h"

//这部分头文件是关于会议室的（群组）
#include "gloox/mucroom.h"
#include "gloox/mucroomhandler.h"
#include "gloox/mucinvitationhandler.h"

//这部分头文件是关于修改自己注册信息的
#include "gloox/vcardhandler.h"
#include "gloox/vcardmanager.h"
#include "gloox/vcard.h"

//这部分头文件是关于实现XEP-069扩展
#include "gloox/siprofileft.h"
#include "gloox/siprofilefthandler.h"
#include "gloox/bytestreamdatahandler.h"
#include "gloox/socks5bytestreamserver.h"

#include "gloox/dataform.h"

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <cstdio>
#include <iostream>
#include <fstream>

//没用其他的数据结构，这里只用了两个命名空间，std和gloox
using namespace std;
using namespace gloox;

//*******************************************************
//gloox一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
//*******************************************************

//*******************************************************
//主要实现对话的virtual interface 
//    "handleMessage"
//在这里，可以收到任何的对话消息，记住，是任何的！
//所以，我们可以利用这个设计，来增加一个xmpp subject类型，表示为语音或者文件消息，
//这个设计在后文中会见到
//*******************************************************

//*******************************************************
//同时需要特别注意对话中的另外一个virtual interface 
//    "handleRosterPresence
//他负责接收好友的出席状态变化，在这里，收到出席状态后，你可以标记出来，很方便
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
                        
            //message body消息体判断，这里判断的原因是 virtusl 实现 handleMessage 句柄后，所有的涉及到
            //xmpp message结构消息的内容都会流经此处
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

        //出席事件状态通知，从这里可以得到联系人的状态改变，从而标记联系人变化
        virtual void handleRosterPresence( const RosterItem& item, const std::string& resource, Presence::PresenceType presence, const std::string& /*msg*/ )
        {
            printf( "presence received: %s/%s -- %d\n", item.jidJID().full().c_str(), resource.c_str(), presence );
        }

        virtual void handleSelfPresence( const RosterItem& item, const std::string& resource, Presence::PresenceType presence, const std::string& /*msg*/ )
        {
            printf( "self presence received: %s/%s -- %d\n", item.jidJID().full().c_str(), resource.c_str(), presence );
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
    MessageTest * r = new MessageTest();
    r->start();
    delete( r );

    return 0;
}
