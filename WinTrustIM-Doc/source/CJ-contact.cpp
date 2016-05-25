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
//这个功能的实现，主要依靠virtual interface:
//     "handleSubscriptionRequest"
//并注意：该接口原先有BUG，我修改了后可以使用，具体修改原因
//请具体的参考gloox的代码
//*******************************************************

//*******************************************************
//添加联系人分为接受邀请，和发出邀请
//接受邀请分成了四个事件，分别为：
// "handleSubscriptionRequest"   收到邀请（原先代码有BUG，导致不能收到邀请，这个函数是修改后的版本）
// "handleItemAdded"             发生添加
// "handleItemUpdated"           花名册发生更新
// "handleItemSubscribed"        好友的出席状态
//按照理解，只需要处理邀请通知，和好友的出席状态通知即可
//*******************************************************

//*******************************************************
//发送邀请
//假设这是一个按钮点击后触发的代码，请求添加好友op23，他的组为Test-Group-Name
//.......
//调用add会同步联系人到远端，本地也有添加，但是通知不会告知给被添加一方，除非调用subscribe发起通知，对端才会收到请求
//StringList groups;
//groups.push_back("Test-Group-Name");
//j->rosterManager()->add( JID( "op23@192.168.199.225" ), "op23", groups ); 
//如果你调用subscribe发起通知，则对端会收到请求
//j->rosterManager()->subscribe( JID(  ) );
//*******************************************************

//*******************************************************
//接触好友关系收到，主要分为3个事件
// "handleItemUnsubscribed"
// "handleItemUpdated"
// "handleItemRemoved"
//按照理解，分别为解除出席，花名册更新，花名册移除
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
        
        virtual void handleItemSubscribed( const JID& jid ) 
        {
            printf( "subscribed %s\n", jid.bare().c_str() );
        }

        virtual void handleItemAdded( const JID& jid )
        {
            printf( "added %s\n", jid.bare().c_str() );
        }

        virtual void handleItemUnsubscribed( const JID& jid )
        {
            printf( "unsubscribed %s\n", jid.bare().c_str() );
        }

        virtual void handleItemRemoved( const JID& jid )
        {
            printf( "removed %s\n", jid.bare().c_str() );
        }

        virtual void handleItemUpdated( const JID& jid )
        {
            printf( "updated %s\n", jid.bare().c_str() );
        }

        virtual bool handleSubscriptionRequest( const JID& jid, const std::string& /*msg*/ )
        {
            //ack {false, true} {拒绝联系人请求，同意联系人请求}
            bool ack = true;
                        
            printf( "subscription: %s\n", jid.bare().c_str() );
            
            //建立一个组为Test-Group-Name
            StringList groups;
            groups.push_back("Test-Group-Name");
            JID id( jid );
            
            //添加到了本地花名册，加入的组为Test-Group-Name
            j->rosterManager()->subscribe( id, "", groups, "" );
            
            return ack;
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
