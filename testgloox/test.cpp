//gloox的类描述
/*
client 实现IO
connectionlistener 实现tcp监听
stanza 实现XMPP响应体描述
registration 实现注册用户
message 实现消息流转
rostermanager 实现花名册
presence 实现联系人状态
mucroom 实现会议室
vcard 实现用户注册信息修改，download

所有的类，里面都包含了virtual接口，如果使用了继承，就必须去实现虚接口
实现后，gloox在你的实现基础之上，还会再次调用他自己的下层实现（virtual）
所以，你只需要负责数据业务逻辑就可以了
*/

//这部分头文件是关于gloox基类，client（IO），连接，stanza（XMPP响应），注册用户，LOG的部分
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

//test.cpp 这个测试代码的来源，主要是gloox提供的test例子，和来自gdb的跟踪信息
//gloox测试例子，提供的内容并不完整，甚至还有bug，比如下面这个：
/*
1661   void ClientBase::notifyMessageHandlers( Message& msg )
1662   {
1663     if( m_mucInvitationHandler )
1664     {
1665       const MUCRoom::MUCUser* mu = msg.findExtension<MUCRoom::MUCUser>( ExtMUCUser );
1666       if( mu && mu->operation() != MUCRoom::OpInviteFrom )
1667       {
1668 
1669         m_mucInvitationHandler->handleMUCInvitation( msg.from(),
1670             mu->jid() ? JID( *(mu->jid()) ) : JID(),
1671             mu->reason() ? *(mu->reason()) : EmptyString,
1672             msg.body(),
1673             mu->password() ? *(mu->password()) : EmptyString,
1674             mu->continued(),
1675             mu->thread() ? *(mu->thread()) : EmptyString );
1676         return;
1677       }
1678     }
*/
//notifyMessageHandlers函数作用是用来判断会议室邀请通知的，注意1666行，mu->operation() 实现了返回当前消息头是否为
//XMPP的invite，也就是邀请，当一个邀请消息经过IO（client类）时候，在这里的状态就是invite，这一点，我通过gdb已经证实了，
//但是，由于!=的操作符错误，这个判断永远会不成立，这种代码，在gloox中，并没有提供测试例子，也许官方都没有测试过，只能是gdb去跟踪流程，
//所以，很多实现，还是需要自己去gdb摸索他的功能
/*
gdb的断点历史记录：
    断点1：gloox::MUCRoom::MUCUser::MUCUser
    断点2：gloox::ClientBase::notifyMessageHandlers
*/

//*******************************************************
//glooc一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
class RegTest : public RegistrationHandler, ConnectionListener, LogHandler
{
    public:
        void start()
        {
            j = new Client( "192.168.199.225" );
            j->disableRoster();
            j->registerConnectionListener( this );
            
            //这个Registration( j )，用来实现一个用户注册
            //这种构造方法，类似MUC，在后面的MUC类实现中有描述，
            //也是用一个基本的client类，这里是 j，来实现IO下的操作
            m_reg = new Registration( j );
            m_reg->registerRegistrationHandler( this );

            j->logInstance().registerLogHandler( LogLevelDebug, LogAreaAll, this );

            j->connect();

            delete( m_reg );
            delete( j );
        }

        virtual void onConnect()
        {
            m_reg->fetchRegistrationFields();
        }
        
        virtual void onDisconnect( ConnectionError e ) { printf( "register_test: disconnected: %d\n", e ); }
        
        virtual bool onTLSConnect( const CertInfo& info )
        {
          printf( "status: %d\nissuer: %s\npeer: %s\nprotocol: %s\nmac: %s\ncipher: %s\ncompression: %s\n",
                  info.status, info.issuer.c_str(), info.server.c_str(),
                  info.protocol.c_str(), info.mac.c_str(), info.cipher.c_str(),
                  info.compression.c_str() );
          return true;
        }
        
        virtual void handleRegistrationFields( const JID& /*from*/, int fields, std::string instructions )
        {
          printf( "fields: %d\ninstructions: %s\n", fields, instructions.c_str() );
          RegistrationFields vals;
          vals.username = "gloox23";
          vals.password = "123456";
          vals.name = "刘组";
          m_reg->createAccount( fields, vals );
        }
        
        virtual void handleRegistrationResult( const JID& /*from*/, RegistrationResult result )
        {
          printf( "result: %d\n", result );
          j->disconnect();
        }

        virtual void handleAlreadyRegistered( const JID& /*from*/ )
        {
          printf( "the account already exists.\n" );
        }

        virtual void handleDataForm( const JID& /*from*/, const DataForm& /*form*/ )
        {
          printf( "datForm received\n" );
        }

        virtual void handleOOB( const JID& /*from*/, const OOB& oob )
        {
          printf( "OOB registration requested. %s: %s\n", oob.desc().c_str(), oob.url().c_str() );
        }

        virtual void handleLog( LogLevel level, LogArea area, const std::string& message )
        {
          printf("log: level: %d, area: %d, %s\n", level, area, message.c_str() );
        }

  private:
        Registration *m_reg;
        Client *j;
};


//*******************************************************
//glooc一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
class MucTest : public MUCRoomHandler, public MUCInvitationHandler
{
    public:
        //构造函数处，这里实现初始化；
        //这里的cli作为参数给了私有成员变量m_client，他是一个client类，在这里用来实现IO
        MucTest(  Client * cli ) : m_room( 0 ), m_client( cli ), MUCInvitationHandler( cli ) {}
    
        virtual ~MucTest() {}
        
        void start()
        {
            JID nick( "glooxMUCtest@conference.192.168.199.225/ca23" );
            m_room = new MUCRoom( m_client, nick, this, 0 );
            DataForm * df = new DataForm( TypeSubmit );
            df->addField( DataFormField::TypeBoolean, "muc#roomconfig_persistentroom", "1", "Make Room Persistent" );
            m_room->join();
            m_room->setRoomConfig(df);
        }
  
        virtual void handleMUCParticipantPresence( MUCRoom * /*room*/, const MUCRoomParticipant participant, const Presence& presence )
        {
            if( presence.presence() == Presence::Available )
                printf( "!!!!!!!!!!!!!!!! %s is in the room, too\n", participant.nick->resource().c_str() );
            else if( presence.presence() == Presence::Unavailable )
                printf( "!!!!!!!!!!!!!!!! %s left the room\n", participant.nick->resource().c_str() );
            else
                printf( "Presence is %d of %s\n", presence.presence(), participant.nick->resource().c_str() );
        }

        //这里获取会议室聊天信息
        virtual void handleMUCMessage( MUCRoom * room, const Message& msg, bool priv )
        {
            printf( "room id %s, %s said: '%s' (history: %s, private: %s)\n", m_muclist.c_str(), msg.from().resource().c_str(), msg.body().c_str(), msg.when() ? "yes" : "no", priv ? "yes" : "no" );
            
            //JID jid( "sp23@192.168.199.225" );
            //Message M( Message::Headline, jid, "headline test", "headline subject" );
            //m_client->send( M );
        }

        virtual void handleMUCSubject( MUCRoom * /*room*/, const std::string& nick, const std::string& subject )
        {
            if( nick.empty() )
                printf( "Subject: %s\n", subject.c_str() );
            else
                printf( "%s has set the subject to: '%s'\n", nick.c_str(), subject.c_str() );
        }

        //这里的错误稍微提一下，常见的错误，有XMPP 400，和XMPP 406
        //400是资源错误，也就是 user@1.2.3.4 后，没有跟随资源，实际应为：user@1.2.3.4/resource
        //resource在不同的XMPP中，会表示不同的作用，在会议室中，他表示昵称
        //406最为常见，是type错误，比如，服务器给你一个groupchat，你回复了一个chat，这就是type错误的一种
        //JID nick( "test@conference.192.168.199.225/glooxmuctest" ); 这就一个完整的例子
        virtual void handleMUCError( MUCRoom * /*room*/, StanzaError error )
        {
            printf( "!!!!!!!!got an error: %d", error );
        }

        virtual void handleMUCInfo( MUCRoom * /*room*/, int features, const std::string& name, const DataForm* infoForm )
        {
            printf( "features: %d, name: %s, form xml: %s\n", features, name.c_str(), infoForm->tag()->xml().c_str() );
        }

        virtual void handleMUCItems( MUCRoom * /*room*/, const Disco::ItemList& items )
        {
            Disco::ItemList::const_iterator it = items.begin();
            for( ; it != items.end(); ++it )
            {
                printf( "%s -- %s is an item here\n", (*it)->jid().full().c_str(), (*it)->name().c_str() );
            }
        }

        virtual void handleMUCInviteDecline( MUCRoom * /*room*/, const JID& invitee, const std::string& reason )
        {
            printf( "Invitee %s declined invitation. reason given: %s\n", invitee.full().c_str(), reason.c_str() );
        }

        virtual bool handleMUCRoomCreation( MUCRoom *room )
        {
            printf( "room %s didn't exist, beeing created.\n", room->name().c_str() );
            return true;
        }
        
        //收到了会议室邀请
        virtual void handleMUCInvitation( const JID& room, const JID& from, const std::string& reason, const std::string& body, const std::string& password, bool cont, const std::string& thread )
        {
            /*
            m_muclist = room.full().c_str();
            printf( "orgi room invict id: %s\n", room.full().c_str() );
            std::string roomid = room.full().c_str();
            roomid.append("/ca23");
            printf( "modify room invict id: %s\n", roomid.c_str() );
            JID nickmeet( roomid );
            m_room = new MUCRoom( m_client, nickmeet, this, 0 );
            m_room->join();
            m_room->getRoomInfo();
            m_room->getRoomItems();
            */
        }
        
    private:
        MUCRoom * m_room;
        Client * m_client;
        std::string m_muclist;
};


//*******************************************************
//glooc一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
//FTTest是用来测试gloox的XEP-069扩展，这个扩展实现了文件传输
class FTTest : public SIProfileFTHandler, BytestreamDataHandler
{
    public:
        FTTest( Client * cli ) : m_bs( 0 ), m_size( 0 ), m_quit( false ), j( cli ) {}
        virtual ~FTTest() {}
        
        void start()
        {
            f = new SIProfileFT( j, this );
            f->addStreamHost( JID( "192.168.199.225" ), "192.168.199.225", 7777 );
        }
        
        virtual void handleFTRequest( const JID& from, const JID& /*to*/, const std::string& sid,
                                  const std::string& name, long size, const std::string& hash,
                                  const std::string& date, const std::string& mimetype,
                                  const std::string& desc, int /*stypes*/ )
        {
            printf( "received ft request from %s: %s (%ld bytes, sid: %s). hash: %s, date: %s, mime-type: %s\n" 
                    "desc: %s\n",
                    from.full().c_str(), name.c_str(), size, sid.c_str(), hash.c_str(), date.c_str(),
                    mimetype.c_str(), desc.c_str() );
            
            f->acceptFT( from, sid, SIProfileFT::FTTypeS5B );
        }
        
        virtual void handleFTRequestError( const IQ& /*iq*/, const std::string& /*sid*/ )
        {
            printf( "ft request error\n" );
            m_quit = true;
        }
        
        virtual void handleFTBytestream( Bytestream * bs )
        {
            printf( "received bytestream of type: %s", bs->type() == Bytestream::S5B ? "s5b" : "ibb" );
            m_bs = ( bs );
            m_bs->registerBytestreamDataHandler( this );
            if( m_bs->connect() )
            {
                if( bs->type() == Bytestream::S5B )
                    printf( "ok! s5b connected to streamhost\n" );
                else
                    printf( "ok! ibb sent request to remote entity\n" );
            }
        }
     
        virtual const std::string handleOOBRequestResult( const JID& /*from*/, const JID& /*to*/, const std::string& /*sid*/ )
        {
            return std::string();
        };

        virtual void handleBytestreamData( Bytestream* /*bs*/, const std::string& data )
        {
            printf( "received %d bytes of data:\n%s\n", data.length(), data.c_str() );
        }

        virtual void handleBytestreamError( Bytestream* /*bs*/, const IQ& /*iq*/ )
        {
            printf( "bytestream error\n" );
        }

        virtual void handleBytestreamOpen( Bytestream* /*bs*/ )
        {
            printf( "bytestream opened\n" );
        }

        virtual void handleBytestreamClose( Bytestream* /*bs*/ )
        {
            printf( "bytestream closed\n" );
            m_quit = true;
        }
        
    private:
        Client * j;
        SIProfileFT * f;
        Bytestream * m_bs;
        SOCKS5BytestreamServer * m_server;
        int m_size;
        bool m_quit;
};


//*******************************************************
//glooc一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
//VCard类用来实现修改添加注册信息，但是不包括修改用户密码，修改密码在注册类里实现
class VCardTest : public VCardHandler
{
    public:
        VCardTest( Client * cli ) : m_client( cli ) {}
        virtual ~VCardTest() {}
        
        //发起修改注册信息，全部为fetchVCard函数操作引起，他的解释是这样的：
        //see this function to fetch the VCard of a remote entity or yourself. 
        //The result will be announced by calling handleVCard() the VCardHandler.
        //看代码也是，fetchVCard函数里面使用IO的（client）的send，导致消息流程开始
        //也就是说，fetchVCard触发后，收到远端送回的数据，触发事件handleVCard，引起修改
        virtual void handleVCard( const JID& jid, const VCard * v )
        {
            if( !v )
            {
                printf( "empty vcard!\n" );
                return;
            }
            
            VCard * vcard = new VCard( *v );
            printf( "received vcard for %s: %s \n", jid.full().c_str(), vcard->tag()->xml().c_str() );
            VCard::AddressList::const_iterator it = vcard->addresses().begin();
            for( ; it != vcard->addresses().end(); ++it )
            {
                printf( "address: %s\n", (*it).street.c_str() );
            }

            VCard * new_vv = new VCard();
            new_vv->addAddress( "pobox", "app. 2", "street", "Springfield", "region", "123", "USA", VCard::AddrTypeHome );
            m_vManager->storeVCard( new_vv, this );
            printf( "setting vcard: %s\n", new_vv->tag()->xml().c_str() );
        }
        
        virtual void handleVCardResult( VCardContext context, const JID& jid, StanzaError se = StanzaErrorUndefined  )
        {
            printf( "vcard result: context: %d, jid: %s, error: %d\n", context, jid.full().c_str(), se );
        }
        
    private:
        Client * m_client;
        VCardManager * m_vManager;
};


//*******************************************************
//glooc一旦实现了继承类，就必须实现这些继承类的virtual接口
//有些接口必须注册（IO里用来通知消息给该接口）
//有的接口则不用注册，在gloox流程分支中会自动触发
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
            
            //从这里我们开始操作会议室的种种功能
            //使用了类MucTest，他继承了MUC类的功能，构造他，然后给他一个client类初始化
            //这个初始化的client类，用于把当前client（client是主要的IO类，实现底层的send，recv）
            //注册进去会议室，这样，会议室的实现类内，就拥有了IO
            //最后一步，用client下的注册事件，注册会议室邀请事件，实际上就是个协议头字符串的判断分支
            //经过这俩步骤，流程变成了
            //（1）实现MUC类，类里拥有了IO，等待这个IO的消息通知
            //（2）IO，也就是client，收到消息，发现自己注册了会议室邀请事件，于是通知注册的类
            //（3）注册的MUC类，收到通知，操作，根据自己的业务逻辑，用IO（client）送出去消息
            //（4）这个类实现了会议邀请，会议创建
            mt = new MucTest( j );
            j->registerMUCInvitationHandler( mt );
            mt->start();
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
                FTTest * r = new FTTest( j );
                r->start();
                delete(r);
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
        MucTest * mt;
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





