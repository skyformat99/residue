//
//  clients.cc
//  Residue
//
//  Copyright 2017-present Muflihun Labs
//
//  Author: @abumusamq
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "cli/clients.h"

#include "core/configuration.h"
#include "core/registry.h"
#include "logging/residue-log-dispatcher.h"
#include "tasks/client-integrity-task.h"
#include "utils/utils.h"

using namespace residue;

Clients::Clients(Registry* registry) :
    Command("clients",
            "List, add or remove clients from the server configuration",
            "clients [list] [clean]",
            registry)
{
}

void Clients::execute(std::vector<std::string>&& params, std::ostringstream& result, bool ignoreConfirmation) const
{
    if (params.empty()) {
        result << "Clients: " << registry()->clients().size() << std::endl;
    }
    if (hasParam(params, "list")) {
        list(result, hasParam(params, "--with-key"));
    } else if (hasParam(params, "clean")) {
        if (registry()->clientIntegrityTask()->isExecuting()) {
            result << "\nAlready running, please try again later" << std::endl;;
            return;
        }

        if (ignoreConfirmation || getConfirmation("This will run client integrity task and clean the expired clients")) {
            registry()->clientIntegrityTask()->kickOff();
            result << "\nFinished client integrity task" << std::endl;
        }
    } else if (hasParam(params, "checkdyn")) {
        // check dynamic buffer
        ResidueLogDispatcher* dispatcher = el::Helpers::logDispatchCallback<ResidueLogDispatcher>("ResidueLogDispatcher");
        if (dispatcher != nullptr) {
            if (dispatcher->m_dynamicBuffer.empty()) {
                result << "Dynamic buffer is empty";
            } else {
                result << "Dynamic buffer information:\n";
                for (auto& pair : dispatcher->m_dynamicBuffer) {
                    result << "Logger: " << pair.second.logger->id() << "\t";
                    result << "Filename: " << pair.first << "\t";
                    result << "Items: " << pair.second.lines.size() << "\n";
                }
            }
        } else {
            result << "Could not extract dispatcher";
        }
    }
}

void Clients::list(std::ostringstream& result, bool withKey) const
{
    int i = 1;
    for (auto& c : registry()->clients()) {
        result << (i++) << " > " << c.second.id()
               << ", Ack: " << (c.second.acknowledged() ? "YES" : "NO")
               << ", Age: " << (Utils::now() - c.second.dateCreated()) << "s"
               << ", Status: "
               << (!c.second.isAlive() ? "DEAD" : "ALIVE " + std::to_string(c.second.age() - (Utils::now() - c.second.dateCreated())) + "s");
        if (withKey) {
            result << ", Key: " << c.second.key();
        }
        if (!c.second.backupKey().empty()) {
            result << ", Backup Key: " << c.second.backupKey();
        }
        result << std::endl;
    }
}
