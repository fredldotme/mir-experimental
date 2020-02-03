/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xcb_connection.h"

#include "mir/log.h"

#include "boost/throw_exception.hpp"

namespace mf = mir::frontend;

mf::XCBConnection::Atom::Atom(std::string const& name, XCBConnection* connection)
    : connection{connection},
      name_{name},
      cookie{xcb_intern_atom(*connection, 0, name_.size(), name_.c_str())}
{
}

mf::XCBConnection::Atom::operator xcb_atom_t() const
{
    if (!atom)
    {
        auto const reply = xcb_intern_atom_reply(*connection, cookie, nullptr);
        if (!reply)
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to look up atom " + name_));
        atom = reply->atom;
        free(reply);
    }
    return atom.value();
}

auto mf::XCBConnection::Atom::name() const -> std::string
{
    return name_;
}

mf::XCBConnection::XCBConnection(int fd)
    : xcb_connection{xcb_connect_to_fd(fd, nullptr)},
      wm_protocols{"WM_PROTOCOLS", this},
      wm_normal_hints{"WM_NORMAL_HINTS", this},
      wm_take_focus{"WM_TAKE_FOCUS", this},
      wm_delete_window{"WM_DELETE_WINDOW", this},
      wm_state{"WM_STATE", this},
      wm_change_state{"WM_CHANGE_STATE", this},
      wm_s0{"WM_S0", this},
      wm_client_machine{"WM_CLIENT_MACHINE", this},
      net_wm_cm_s0{"_NET_WM_CM_S0", this},
      net_wm_name{"_NET_WM_NAME", this},
      net_wm_pid{"_NET_WM_PID", this},
      net_wm_icon{"_NET_WM_ICON", this},
      net_wm_state{"_NET_WM_STATE", this},
      net_wm_state_maximized_vert{"_NET_WM_STATE_MAXIMIZED_VERT", this},
      net_wm_state_maximized_horz{"_NET_WM_STATE_MAXIMIZED_HORZ", this},
      net_wm_state_hidden{"_NET_WM_STATE_HIDDEN", this},
      net_wm_state_fullscreen{"_NET_WM_STATE_FULLSCREEN", this},
      net_wm_user_time{"_NET_WM_USER_TIME", this},
      net_wm_icon_name{"_NET_WM_ICON_NAME", this},
      net_wm_desktop{"_NET_WM_DESKTOP", this},
      net_wm_window_type{"_NET_WM_WINDOW_TYPE", this},
      net_wm_window_type_desktop{"_NET_WM_WINDOW_TYPE_DESKTOP", this},
      net_wm_window_type_dock{"_NET_WM_WINDOW_TYPE_DOCK", this},
      net_wm_window_type_toolbar{"_NET_WM_WINDOW_TYPE_TOOLBAR", this},
      net_wm_window_type_menu{"_NET_WM_WINDOW_TYPE_MENU", this},
      net_wm_window_type_utility{"_NET_WM_WINDOW_TYPE_UTILITY", this},
      net_wm_window_type_splash{"_NET_WM_WINDOW_TYPE_SPLASH", this},
      net_wm_window_type_dialog{"_NET_WM_WINDOW_TYPE_DIALOG", this},
      net_wm_window_type_dropdown{"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", this},
      net_wm_window_type_popup{"_NET_WM_WINDOW_TYPE_POPUP_MENU", this},
      net_wm_window_type_tooltip{"_NET_WM_WINDOW_TYPE_TOOLTIP", this},
      net_wm_window_type_notification{"_NET_WM_WINDOW_TYPE_NOTIFICATION", this},
      net_wm_window_type_combo{"_NET_WM_WINDOW_TYPE_COMBO", this},
      net_wm_window_type_dnd{"_NET_WM_WINDOW_TYPE_DND", this},
      net_wm_window_type_normal{"_NET_WM_WINDOW_TYPE_NORMAL", this},
      net_wm_moveresize{"_NET_WM_MOVERESIZE", this},
      net_supporting_wm_check{"_NET_SUPPORTING_WM_CHECK", this},
      net_supported{"_NET_SUPPORTED", this},
      net_active_window{"_NET_ACTIVE_WINDOW", this},
      motif_wm_hints{"_MOTIF_WM_HINTS", this},
      clipboard{"CLIPBOARD", this},
      clipboard_manager{"CLIPBOARD_MANAGER", this},
      targets{"TARGETS", this},
      utf8_string{"UTF8_STRING", this},
      wl_selection{"_WL_SELECTION", this},
      incr{"INCR", this},
      timestamp{"TIMESTAMP", this},
      multiple{"MULTIPLE", this},
      compound_text{"COMPOUND_TEXT", this},
      text{"TEXT", this},
      string{"STRING", this},
      window{"WINDOW", this},
      text_plain_utf8{"text/plain;charset=utf-8", this},
      text_plain{"text/plain", this},
      xdnd_selection{"XdndSelection", this},
      xdnd_aware{"XdndAware", this},
      xdnd_enter{"XdndEnter", this},
      xdnd_leave{"XdndLeave", this},
      xdnd_drop{"XdndDrop", this},
      xdnd_status{"XdndStatus", this},
      xdnd_finished{"XdndFinished", this},
      xdnd_type_list{"XdndTypeList", this},
      xdnd_action_copy{"XdndActionCopy", this},
      wl_surface_id{"WL_SURFACE_ID", this},
      allow_commits{"_XWAYLAND_ALLOW_COMMITS", this}
{
}

mf::XCBConnection::~XCBConnection()
{
    xcb_disconnect(xcb_connection);
}

mf::XCBConnection::operator xcb_connection_t*() const
{
    return xcb_connection;
}

auto mf::XCBConnection::query_name(xcb_atom_t atom) -> std::string
{
    // TODO: cache, for cheaper lookup

    if (atom == XCB_ATOM_NONE)
        return "None";

    xcb_get_atom_name_cookie_t const cookie = xcb_get_atom_name(xcb_connection, atom);
    xcb_get_atom_name_reply_t* const reply = xcb_get_atom_name_reply(xcb_connection, cookie, nullptr);

    std::string name;

    if (reply)
    {
        name = std::string{xcb_get_atom_name_name(reply), static_cast<size_t>(xcb_get_atom_name_name_length(reply))};
    }
    else
    {
        name = "Atom " + std::to_string(atom);
    }

    free(reply);

    return name;
}

auto mf::XCBConnection::reply_contains_string_data(xcb_get_property_reply_t const* reply) -> bool
{
    return reply->type == XCB_ATOM_STRING || reply->type == utf8_string;
}

auto mf::XCBConnection::string_from(xcb_get_property_reply_t const* reply) -> std::string
{
    if (!reply_contains_string_data(reply))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Supplied reply is of type " + query_name(reply->type) + " and does not hold string data"));
    }

    return std::string{
        static_cast<const char *>(xcb_get_property_value(reply)),
        static_cast<unsigned long>(xcb_get_property_value_length(reply))};
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(xcb_get_property_reply_t*)> action) -> std::function<void()>
{
    xcb_get_property_cookie_t cookie = xcb_get_property(
        xcb_connection,
        0, // don't delete
        window,
        prop,
        XCB_ATOM_ANY,
        0, // no offset
        2048); // big buffer

    return [xcb_connection = xcb_connection, cookie, action]()
        {
            xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection, cookie, nullptr);
            if (reply && reply->type != XCB_ATOM_NONE)
            {
                try
                {
                    action(reply);
                }
                catch (...)
                {
                    log(
                        logging::Severity::warning,
                        MIR_LOG_COMPONENT,
                        std::current_exception(),
                        "Failed to process property reply.");
                }
            }
            free(reply);
        };
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(std::string const&)> action) -> std::function<void()>
{
    return read_property(window, prop, [this, action](xcb_get_property_reply_t const* reply)
        {
            action(string_from(reply));
        });
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(uint32_t)> action) -> std::function<void()>
{
    return read_property(window, prop, [this, action](xcb_get_property_reply_t const* reply)
        {
            if (reply->format != 32)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Reply of type " + query_name(reply->type) +
                    " has a format " + std::to_string(reply->format) +
                    " instead of expected 32"));
            }

            if (xcb_get_property_value_length(reply) != 1)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Reply of type " + query_name(reply->type) +
                    " has a value length " + std::to_string(xcb_get_property_value_length(reply)) +
                    " instead of expected 1"));
            }

            action(*static_cast<uint32_t const*>(xcb_get_property_value(reply)));
        });
}

auto mf::XCBConnection::read_property(
    xcb_window_t window,
    xcb_atom_t prop,
    std::function<void(std::vector<uint32_t>)> action) -> std::function<void()>
{
    return read_property(window, prop, [this, action](xcb_get_property_reply_t const* reply)
        {
            if (reply->format != 32)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Reply of type " + query_name(reply->type) +
                    " has a format " + std::to_string(reply->format) +
                    " instead of expected 32"));
            }

            auto const start = static_cast<uint32_t const*>(xcb_get_property_value(reply));
            auto const end = start + static_cast<size_t>(xcb_get_property_value_length(reply));
            std::vector<uint32_t> const values{start, end};

            action(values);
        });
}

auto mf::XCBConnection::xcb_type_atom(XCBType type) const -> xcb_atom_t
{
    switch (type)
    {
        case XCBType::ATOM:         return XCB_ATOM_ATOM;
        case XCBType::WINDOW:       return XCB_ATOM_WINDOW;
        case XCBType::CARDINAL32:   return XCB_ATOM_CARDINAL;
        case XCBType::STRING:       return XCB_ATOM_STRING;
        case XCBType::UTF8_STRING:  return utf8_string;
        case XCBType::WM_STATE:     return wm_state;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error(
        "Invalid XCB type " +
        std::to_string(static_cast<std::underlying_type<XCBType>::type>(type))));
}