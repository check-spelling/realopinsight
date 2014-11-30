/*
 * User.hpp
# ------------------------------------------------------------------------ #
# Copyright (c) 2010-2014 Rodrigue Chakode (rodrigue.chakode@gmail.com)    #
# Last Update: 23-03-2014                                                  #
#                                                                          #
# This file is part of RealOpInsight (http://RealOpInsight.com) authored   #
# by Rodrigue Chakode <rodrigue.chakode@gmail.com>                         #
#                                                                          #
# RealOpInsight is free software: you can redistribute it and/or modify    #
# it under the terms of the GNU General Public License as published by     #
# the Free Software Foundation, either version 3 of the License, or        #
# (at your option) any later version.                                      #
#                                                                          #
# The Software is distributed in the hope that it will be useful,          #
# but WITHOUT ANY WARRANTY; without even the implied warranty of           #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
# GNU General Public License for more details.                             #
#                                                                          #
# You should have received a copy of the GNU General Public License        #
# along with RealOpInsight.  If not, see <http://www.gnu.org/licenses/>.   #
#--------------------------------------------------------------------------#
 */

#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <list>
#include <Wt/Dbo/Dbo>
#include <string>
#include <set>
#include <Wt/WDateTime>
#include <QString>
#include <QMap>

namespace dbo = Wt::Dbo;

/** holds View info like a wt::dbo object*/
class DboView;

/** holds User info like a wt::dbo object*/
class DboUser;

/** holds LoginSession info like a wt::dbo object*/
class DboLoginSession;

/** holds QoS data without wt::dbo specific properties*/
struct QosDataT;

/** holds QoS data info like a wt::dbo object*/
class DboQosData;

/** holds Notification info like a wt::dbo object*/
class DboNotification;

/** holds Notification info without dbo specific info*/
struct NotificationT;

namespace Wt {
  namespace Dbo {
    template<>
    struct dbo_traits<DboView> : public dbo_default_traits
    {
      typedef std::string IdType;
      static IdType invalidId() { return std::string(); }
      static const char* surrogateIdField() { return 0; }
    };

    template<>
    struct dbo_traits<DboUser> : public dbo_default_traits
    {
      typedef std::string IdType;
      static IdType invalidId() { return std::string(); }
      static const char* surrogateIdField() { return 0; }
    };
  }
}

class DboView
{
public:
  std::string name;
  std::string path;
  int service_count;
  dbo::collection< dbo::ptr<DboUser> > users;
  dbo::collection< dbo::ptr<DboQosData> > qosdatas;
  dbo::collection< dbo::ptr<DboNotification> > notifications;

  template<class Action>
  void persist(Action& a) {
    dbo::id(a, name, "name");
    dbo::field(a, path, "path");
    dbo::field(a, service_count, "service_count");
    dbo::hasMany(a, users, dbo::ManyToMany, "user_view");
    dbo::hasMany(a, qosdatas, dbo::ManyToOne, "view");
    dbo::hasMany(a, notifications, dbo::ManyToOne, "view");
  }
};


class DboUser {
public:
  enum RoleT {
    AdmRole = 100,
    OpRole = 101
  };

  std::string username;
  std::string password;
  std::string firstname;
  std::string lastname;
  std::string email;
  int role;
  std::string registrationDate;
  int authsystem; // e.g. LDAP or BuiltIn
  dbo::collection< dbo::ptr<DboView> > views;
  dbo::collection< dbo::ptr<DboLoginSession> > sessions;
  dbo::collection< dbo::ptr<DboNotification> > ack_notifications;

  template<class Action>
  void persist(Action& a) {
    dbo::id(a, username, "name");
    dbo::field(a, firstname, "firstname");
    dbo::field(a, lastname, "lastname");
    dbo::field(a, email, "email");
    dbo::field(a, role, "role");
    dbo::field(a, registrationDate, "registrationDate");
    dbo::field(a, authsystem, "authsystem");
    dbo::hasMany(a, views, dbo::ManyToMany, "user_view");
    dbo::hasMany(a, sessions, dbo::ManyToMany);
    dbo::hasMany(a, ack_notifications, dbo::ManyToOne, "ack_user");
  }

  static std::string role2Text(int role) {
    return role == AdmRole? "Administrator" : "Operator";
  }
  static int role2Int(const std::string& role) {
    return role == "Administrator" ? AdmRole : OpRole;
  }
};

/** holds QoS data without wt::dbo specific info */
struct QosDataT {
  long timestamp;
  int status;
  float normal;
  float minor;
  float major;
  float critical;
  float unknown;
  std::string view_name;

  std::string toString(void) const {
    return QString("%1,%2,%3,%4,%5,%6,%7, %8")
        .arg(QString::number(timestamp),
             view_name.c_str(),
             QString::number(status),
             QString::number(normal),
             QString::number(minor),
             QString::number(major),
             QString::number(critical),
             QString::number(unknown)).toStdString();
  }
};


/** holds QoS data like wt::dbo class */
class DboQosData {
public:
  long timestamp;
  int status;
  float normal;
  float minor;
  float major;
  float critical;
  float unknown;
  dbo::ptr<DboView> view;

  void setData(const QosDataT& data)
  {
    timestamp = data.timestamp;
    status = data.status;
    normal = data.normal;
    minor = data.minor;
    major = data.major;
    critical = data.critical;
    unknown = data.unknown;
  }

  QosDataT data(void) const
  {
    QosDataT d;
    d.timestamp = timestamp;
    d.status = status;
    d.normal = normal;
    d.minor = minor;
    d.major = major;
    d.critical = critical;
    d.unknown = unknown;
    d.view_name = view ? view->name : "";
    return d;
  }

  template<class Action>
  void persist(Action& a) {
    dbo::field(a, timestamp, "timestamp");
    dbo::field(a, status, "status");
    dbo::field(a, normal, "normal");
    dbo::field(a, minor, "minor");
    dbo::field(a, major, "major");
    dbo::field(a, critical, "critical");
    dbo::field(a, unknown, "unknown");
    dbo::belongsTo(a, view, "view");
  }

  std::string toString(void) const {
    return QString("%1,%2,%3,%4,%5,%6,%7, %8")
        .arg(QString::number(timestamp),
             view->name.c_str(),
             QString::number(status),
             QString::number(normal),
             QString::number(minor),
             QString::number(major),
             QString::number(critical),
             QString::number(unknown)).toStdString();
  }
};

class DboLoginSession
{
public:
  enum {
    ExpiredCookie = 0,
    ActiveCookie = 1,
    InvalidSession = 2
  };

  std::string username;
  std::string sessionId;
  std::string firstAccess;
  std::string lastAccess;
  int status;

  template<class Action>
  void persist(Action& a) {
    dbo::field(a, username, "username");
    dbo::field(a, sessionId, "session_id");
    dbo::field(a, firstAccess, "first_access");
    dbo::field(a, lastAccess, "last_access");
    dbo::field(a, status, "status");
  }
};

/** holds notification info without wt::dbo specific properties (e.g dbo pointers)*/
struct NotificationT {
  long timestamp;
  int view_status;
  long last_change;
  long ack_status;
  std::string view_name;
  std::string ack_username;
};

/** holds notification info like a wt::dbo object */
class DboNotification
{
public:
  enum AckStatusT {
    Closed = 0,
    Open = 1,
    Acknowledged = 2
  };

  long timestamp;
  int view_status;
  int ack_status;
  long last_change;
  dbo::ptr<DboView> view;
  dbo::ptr<DboUser> ack_user;

  NotificationT data(void) const
  {
    NotificationT d;
    d.timestamp = timestamp;
    d.view_name = view? view->name : "";
    d.view_status = view_status;
    d.ack_status = ack_status;
    d.last_change = last_change;
    d.ack_username = ack_user ? ack_user->username : "";
    return d;
  }

  template<class Action>
  void persist(Action& a) {
    dbo::field(a, timestamp, "timestamp");
    dbo::field(a, view_status, "view_status");
    dbo::field(a, ack_status, "ack_status");
    dbo::field(a, last_change, "last_change");
    dbo::belongsTo(a, view, "view");
    dbo::belongsTo(a, ack_user, "ack_user");
  }
};

typedef std::set<std::string> UserViewsT;
typedef std::list<DboUser> DbUsersT;
typedef std::list<DboView> DbViewsT;
typedef std::list<DboLoginSession> LoginSessionListT;
typedef std::list<QosDataT> QosDataList;
typedef std::list<NotificationT> NotificationListT;
typedef QMap<std::string, std::list<QosDataT> > QosDataByViewMapT;
typedef dbo::collection< dbo::ptr<DboUser> > DboUserCollectionT;
typedef dbo::collection< dbo::ptr<DboView> > DboViewCollectionT;
typedef dbo::collection< dbo::ptr<DboQosData> > DboQosDataCollectionT;
typedef dbo::collection< dbo::ptr<DboNotification> > DboNotificationCollectionT;
typedef dbo::collection< dbo::ptr<DboLoginSession> > DboLoginSessionCollectionT;

#endif // USER_HPP
