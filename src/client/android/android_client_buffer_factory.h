/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_FACTORY_H_
#define MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_FACTORY_H_

#include "client_buffer_factory.h"

namespace mir
{
namespace client
{
class ClientBuffer;
class AndroidRegistrar;

class AndroidClientBufferFactory : public ClientBufferFactory
{
public:
    AndroidClientBufferFactory(const std::shared_ptr<AndroidRegistrar>&);
    std::shared_ptr<ClientBuffer> create_buffer_from_ipc_message(const std::shared_ptr<MirBufferPackage>&,
                                geometry::Width, geometry::Height, geometry::PixelFormat);
 
private:
    std::shared_ptr<AndroidRegistrar> registrar;
};
}
}
#endif /* MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_FACTORY_H_ */
