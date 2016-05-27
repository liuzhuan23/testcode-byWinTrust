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
//MUC类，也就是会议室功能类
//实现群组聊天必须继承MUC类
//*******************************************************
class MucTest : public MUCRoomHandler, public MUCInvitationHandler
{
    public:
        //构造函数处，这里实现初始化；
        //这里的cli作为参数给了私有成员变量m_client，他是一个client类，在这里用来实现IO
        MucTest(  Client * cli ) : m_room( 0 ), m_client( cli ), MUCInvitationHandler( cli ) {}
    
        virtual ~MucTest() {}
        
        void start()
        {
            //在start方法中，主动建立一个会议室，会议室的资源名称为：glooxMUCtest
            //进入会议室后的别名是ca23
            //XMPP会议室，必须要求具备会议室资源名称，并配置自己的别名
            //资源名称@conference.IP地址/会议室内别名
            JID nick( "glooxMUCtest@conference.192.168.199.225/ca23" );
            m_room = new MUCRoom( m_client, nick, this, 0 );
            //DataForm是Gloox内的一个数据类，里面有一些宏标记，这里我们使用TypeSubmit
            DataForm * df = new DataForm( TypeSubmit );
            //设置标记muc#roomconfig_persistentroom，这个是XMPP XEP标准，含义是永久性会议室
            //XMPP会议室分为两种：临时性，永久性，临时的只要建立用户退出来，会议室自动消失
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

        //这里获取会议室聊天信息，也就是说，在这里获得群组聊天信息
        virtual void handleMUCMessage( MUCRoom * room, const Message& msg, bool priv )
        {
            printf( "room id %s, %s said: '%s' (history: %s, private: %s)\n", m_muclist.c_str(), msg.from().resource().c_str(), msg.body().c_str(), msg.when() ? "yes" : "no", priv ? "yes" : "no" );
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
        //特别提醒一下，这个虚函数handleMUCInvitation在Gloox原版本中，是有bug的，这个函数是我修改之后的版本
        //原版本，会收不到会议邀请
        virtual void handleMUCInvitation( const JID& room, const JID& from, const std::string& reason, const std::string& body, const std::string& password, bool cont, const std::string& thread )
        {
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
        }
        
    private:
        MUCRoom * m_room;
        Client * m_client;
        std::string m_muclist;
};



int main( int argc, char* argv[] )
{
    MessageTest * r = new MessageTest();
    r->start();
    delete( r );

    return 0;
}

