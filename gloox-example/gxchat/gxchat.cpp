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

class MessageTest : RosterListener, MessageSessionHandler, ConnectionListener, MessageEventHandler, MessageHandler, ChatStateHandler, LogHandler
{
    public:
        MessageTest() : m_session( 0 ), m_messageEventFilter( 0 ), m_chatStateFilter( 0 ) {}

        virtual ~MessageTest() {}

        void start()
        {
            JID jid( "test456@192.168.199.103/test456" );
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
            if( msg.body() == "test-quit" )
            {
                j->disconnect();
            }
            else if( msg.body() == "test-subscribe" )
            {
                j->rosterManager()->subscribe( msg.from() );
            }
            else if( msg.body() == "test-unsubscribe" )
            {
                j->rosterManager()->unsubscribe( msg.from() );
            }
            else if( msg.body() == "test-cancel" )
            {
                j->rosterManager()->cancel( msg.from() );
            }
            else if( msg.body() == "test-remove" )
            {
                j->rosterManager()->remove( msg.from() );
            }
            else if( msg.body() == "test-buddylist" ) //遍历本地花名册
            {
                Roster * roster = j->rosterManager()->roster();
                Roster::const_iterator it = roster->begin();
                for( ; it != roster->end(); ++it )
                {
                    printf( "jid: %s, name: %s, subscription: %d\n", (*it).second->jidJID().full().c_str(), (*it).second->name().c_str(), (*it).second->subscription() );
                
                    StringList g = (*it).second->groups();
                    StringList::const_iterator it_g = g.begin();
                
                    for( ; it_g != g.end(); ++it_g )
                    {
                        printf( "\tgroup: %s\n", (*it_g).c_str() );
                    }
                
                    RosterItem::ResourceMap::const_iterator rit = (*it).second->resources().begin();
                    for( ; rit != (*it).second->resources().end(); ++rit )
                    {
                        printf( "resource: %s\n", (*rit).first.c_str() );
                    }
                }
            }
            else if ( msg.body() == "test-addop23" ) //请求添加联系人op23
            {
                //只是调用add会同步联系人到远端，本地也有添加，但是通知不会告知给被添加一方
                StringList groups;
                groups.push_back("Test-Group-Name");
                j->rosterManager()->add( JID( "op23@192.168.199.225" ), "op23", groups ); 
                //如果你调用subscribe发起通知，则对端会收到请求
                j->rosterManager()->subscribe( JID(  ) );
            }
            else if ( msg.body() == "test-bin" ) 
            {
                //测试二进制数据，我这里使用了一个B64编码的文件，
                //该文件经过了BASE64编码，原先为一个图片
                /*
                MessageType
                Chat 	     A chat message.
                Error 	     An error message.
                Groupchat 	 A groupchat message.
                Headline 	 A headline message.
                Normal 	     A normal message.
                Invalid 	 The message stanza is invalid. 
                */
                std::ifstream in("input.txt", ios::in);
                std::istreambuf_iterator<char> beg(in), end;
                std::string strdata(beg, end);
                in.close();
                std::string subject = "VOICE";
                m_session->send( strdata, subject );
            }
            else if ( msg.body() == "test-pssd" )
            {
                //自己的定义的修改密码，修改密码不能使用VCard类，Vcard是修改添加注册信息的
                //这里需要使用 Registration 类
                Registration * m_reg;
                m_reg = new Registration( j );
                m_reg->fetchRegistrationFields();
                // changing password
                m_reg->changePassword( j->username(), "123456" );
                delete( m_reg );
            }
            else if ( msg.body() == "test-headline" )
            {
                //这里测试headline的直接send，注意这里没有使用 MessageSession * m_session; 的send，他只能送字符串
                //这里使用的是Client * j; 的send，他可以送出去HeadLine这个类
                //其实send最后都走到了Client IO的send，区别就是类功能的实现和继承方法
                Message M( Message::Headline, j->jid(), "headline test", "headline subject" );
                j->send( M );
            }
            else if ( msg.body() == "test-ft" )
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

        virtual void handleItemSubscribed( const JID& jid ) //本地添加好友添加完成最后一步（添加联系人-4）
        {
            printf( "subscribed %s\n", jid.bare().c_str() );
        }

        virtual void handleItemAdded( const JID& jid ) //add事件发生（添加联系人-2）
        {
            printf( "added %s\n", jid.bare().c_str() );
        }

        virtual void handleItemUnsubscribed( const JID& jid ) //收到解除好友关系（解除联系人-1）
        {
            printf( "unsubscribed %s\n", jid.bare().c_str() );
        }

        virtual void handleItemRemoved( const JID& jid ) //移除好友关系（解除联系人-3）
        {
            printf( "removed %s\n", jid.bare().c_str() );
        }

        virtual void handleItemUpdated( const JID& jid ) //发生了更新本地好友列表（添加联系人-3）, （解除联系人-2）
        {
            printf( "updated %s\n", jid.bare().c_str() );
        }

        virtual void handleRoster( const Roster& roster ) //激活本地花名册
        {
            printf( "roster arriving\n" );
        }

        virtual void handleRosterError( const IQ& /*iq*/ )
        {
            printf( "a roster-related error occured\n" );
        }
        
        virtual bool handleUnsubscriptionRequest( const JID& jid, const std::string& /*msg*/ )
        {
            printf( "unsubscription: %s\n", jid.bare().c_str() );
            return true;
        }

        virtual void handleNonrosterPresence( const Presence& presence )
        {
            printf( "received presence from entity not in the roster: %s\n", presence.from().full().c_str() );
        }

        ////出席事件状态通知，从这里可以得到联系人的状态改变，从而标记联系人变化
        virtual void handleRosterPresence( const RosterItem& item, const std::string& resource, Presence::PresenceType presence, const std::string& /*msg*/ )
        {
            printf( "presence received: %s/%s -- %d\n", item.jidJID().full().c_str(), resource.c_str(), presence );
        }

        virtual void handleSelfPresence( const RosterItem& item, const std::string& resource, Presence::PresenceType presence, const std::string& /*msg*/ )
        {
            printf( "self presence received: %s/%s -- %d\n", item.jidJID().full().c_str(), resource.c_str(), presence );
        }

        virtual bool handleSubscriptionRequest( const JID& jid, const std::string& /*msg*/ ) //发生收到好友请求（添加联系人-1）
        {
            //ack {false, true} {拒绝联系人请求，同意联系人请求}
            bool ack = true;
                        
            printf( "subscription: %s\n", jid.bare().c_str() );
            StringList groups;
            groups.push_back("Test-Group-Name");
            JID id( jid );
            //添加到了本地花名册
            j->rosterManager()->subscribe( id, "", groups, "" );
            
            return ack;
        }

    private:
        Client * j;
        MessageSession * m_session;
        MessageEventFilter * m_messageEventFilter;
        ChatStateFilter * m_chatStateFilter;
};


//*******************************************************
//*******************************************************
int main( int argc, char* argv[] )
{
    //测试注册用户
    /*
    RegTest *r = new RegTest();
    r->start();
    delete( r );
    */
    
    //测试消息，包括聊天室，文件传输，语音消息，图片等，具体参考消息命令
    MessageTest * r = new MessageTest();
    r->start();
    delete( r );

    return 0;
}

