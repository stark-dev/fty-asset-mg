/*  =========================================================================
    fty_common_asset - General asset representation

    Copyright (C) 2019 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_common_asset - General asset representation
@discuss
@end
*/

#include "fty_common_asset.h"

#include <cxxtools/jsonserializer.h>
#include <cxxtools/jsondeserializer.h>
#include <fty_common_mlm.h>
#include <fty_log.h>
#include <ftyproto.h>
#include <fty/convert.h>

namespace fty {
    BasicAsset::BasicAsset ()
    { }

    BasicAsset::BasicAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype)
        : id_(id), status_(stringToStatus (status)),
          type_subtype_(std::make_pair (stringToType (type), stringToSubtype (subtype)))
    { }

    BasicAsset::BasicAsset (fty_proto_t *msg)
    {
        if (fty_proto_id (msg) != FTY_PROTO_ASSET)
            throw std::invalid_argument ("Wrong fty-proto type");
        id_ = fty_proto_name (msg);
        std::string status_str = fty_proto_aux_string (msg, "status", "active");
        status_ = stringToStatus (status_str);
        std::string type_str = fty_proto_aux_string (msg, "type", "");
        std::string subtype_str = fty_proto_aux_string (msg, "subtype", "");
        type_subtype_ = std::make_pair (stringToType (type_str), stringToSubtype (subtype_str));
    }


    BasicAsset::BasicAsset (const cxxtools::SerializationInfo & si)
    {
        si >>= *this;
    }

    void BasicAsset::deserialize (const cxxtools::SerializationInfo & si)
    {
        std::string status_str;
        if (!si.findMember ("status"))
        {
            throw std::runtime_error("status");
        }
        si.getMember("status") >>= status_str;
        if (status_str.empty())
        {
            throw std::runtime_error("status");
        }

        status_ = stringToStatus (status_str);

        if (!si.findMember ("type"))
        {
            throw std::runtime_error("type");
        }
        if (!si.findMember ("sub_type"))
        {
            throw std::runtime_error("sub_type");
        }

        std::string type_str, subtype_str;
        si.getMember("type") >>= type_str;
        if (type_str.empty())
        {
            throw std::runtime_error("type");
        }
        si.getMember("sub_type") >>= subtype_str;
        type_subtype_ = std::make_pair (stringToType (type_str), stringToSubtype (subtype_str));

        if (si.findMember ("id"))
        {
            si.getMember("id") >>= id_;
        }
        else // use type/subtype as preliminary ID, as is done elsewhere
        {
            if (type_str == "device")
            {
                id_ = subtype_str;
            }
            else
            {
                id_ = type_str;
            }
        }
    }

    void BasicAsset::serialize (cxxtools::SerializationInfo & si) const
    {
        si.addMember("id") <<= id_;

        std::string status_str = statusToString (status_);
        si.addMember("status") <<= status_str;

        std::string type_str = typeToString (type_subtype_.first);
        std::string subtype_str = subtypeToString (type_subtype_.second);
        si.addMember("type") <<= type_str;
        si.addMember("sub_type") <<= subtype_str;
    }

    bool BasicAsset::operator == (const BasicAsset &asset) const
    {
        return id_ == asset.id_ && status_ == asset.status_ && type_subtype_ == asset.type_subtype_;
    }

    bool BasicAsset::operator!= (const BasicAsset &asset) const
    {
        return !(*this == asset);
    }

    /// comparator for simplified equality check
    bool BasicAsset::compare (const BasicAsset &asset) const
    {
        return asset.id_ == id_;
    }

    // getters/setters
    std::string BasicAsset::getId () const
    {
        return id_;
    }

    BasicAsset::Status BasicAsset::getStatus () const
    {
        return status_;
    }

    std::string BasicAsset::getStatusString () const
    {
        return statusToString (status_);
    }

    BasicAsset::Type BasicAsset::getType () const
    {
        return type_subtype_.first;
    }

    std::string BasicAsset::getTypeString () const
    {
        return typeToString (type_subtype_.first);
    }

    BasicAsset::Subtype BasicAsset::getSubtype () const
    {
        return type_subtype_.second;
    }

    std::string BasicAsset::getSubtypeString () const
    {
        return subtypeToString (type_subtype_.second);
    }

    void BasicAsset::setStatus (const std::string & status)
    {
        status_ = stringToStatus (status);
    }

    void BasicAsset::setType (const std::string & type)
    {
        type_subtype_.first = stringToType (type);
    }

    void BasicAsset::setSubtype (const std::string & subtype)
    {
        type_subtype_.second = stringToSubtype (subtype);
    }

    std::string BasicAsset::typeToString (BasicAsset::Type type) const
    {
        switch (type) {
            case Type_Cluster:
                return "cluster";
            case Type_Datacenter:
                return "datacenter";
            case Type_Device:
                return "device";
            case Type_Group:
                return "group";
            case Type_Hypervisor:
                return "hypervisor";
            case Type_Rack:
                return "rack";
            case Type_Room:
                return "room";
            case Type_Row:
                return "row";
            case Type_Storage:
                return "storage";
            case Type_VApp:
                return "vapp";
            case Type_VirtuService:
                return "virtuservice";
            case Type_VM:
                return "vm";
            case Type_COPS:
                return "cops";
            default:
                throw std::invalid_argument ("type id is not known value");
        }
    }

    BasicAsset::Type BasicAsset::stringToType (const std::string & type) const
    {
        if (type == "cluster") {
            return Type_Cluster;
        } else if (type == "datacenter") {
            return Type_Datacenter;
        } else if (type == "device") {
            return Type_Device;
        } else if (type == "group") {
            return Type_Group;
        } else if (type == "hypervisor") {
            return Type_Hypervisor;
        } else if (type == "rack") {
            return Type_Rack;
        } else if (type == "room") {
            return Type_Room;
        } else if (type == "row") {
            return Type_Row;
        } else if (type == "storage") {
            return Type_Storage;
        } else if (type == "vapp") {
            return Type_VApp;
        } else if (type == "virtuservice") {
            return Type_VirtuService;
        } else if (type == "vm") {
            return Type_VM;
        } else if (type == "cops") {
            return Type_COPS;
        } else {
            throw std::invalid_argument ("type is not known value");
        }
    }

    std::string BasicAsset::subtypeToString (BasicAsset::Subtype subtype) const
    {
        switch (subtype) {
            case Subtype_Appliance:
                return "appliance";
            case Subtype_Chassis:
                return "chassis";
            case Subtype_CitrixPool:
                return "citrix.pool";
            case Subtype_CitrixTask:
                return "citrix.task";
            case Subtype_CitrixVApp:
                return "citrix.vapp";
            case Subtype_CitrixVM:
                return "citrix.vm";
            case Subtype_CitrixXenserver:
                return "citrix.xenserver";
            case Subtype_EPDU:
                return "epdu";
            case Subtype_Feed:
                return "feed";
            case Subtype_Genset:
                return "genset";
            case Subtype_GPO:
                return "gpo";
            case Subtype_HPITManager:
                return "hp.it.manager";
            case Subtype_HPITManagerService:
                return "hp.it.manager.service";
            case Subtype_HPITRack:
                return "hp.it.rack";
            case Subtype_HPITServer:
                return "hp.it.server";
            case Subtype_IPMInfraServer:
                return "ipminfra.server";
            case Subtype_IPMInfraService:
                return "ipminfra.service";
            case Subtype_MicrosoftCluster:
                return "microsoft.cluster";
            case Subtype_MicrosoftHyperV:
                return "microsoft.hyperv";
            case Subtype_MicrosoftServer:
                return "microsoft.server";
            case Subtype_MicrosoftTask:
                return "microsoft.task";
            case Subtype_MicrosoftVirtualizationMachine:
                return "microsoft.virtualization.machine";
            case Subtype_MicrosoftVM:
                return "microsoft.vm";
            case Subtype_MicrosoftWindowsServer:
                return "microsoft.windows.server";
            case Subtype_NetAppCluster:
                return "netapp.cluster";
            case Subtype_NetAppNode:
                return "netapp.node";
            case Subtype_NetAppOntapNode:
                return "netapp.ontap.node";
            case Subtype_NetAppOntapSystem:
                return "netapp.ontap.system";
            case Subtype_NetAppServer:
                return "netapp.server";
            case Subtype_NutanixCluster:
                return "nutanix.cluster";
            case Subtype_NutanixNode:
                return "nutanix.node";
            case Subtype_NutanixPrismGateway:
                return "nutanix.prism.gateway";
            case Subtype_NutanixVirtualizationMachine:
                return "nutanix.virtualization.machine";
            case Subtype_N_A:
                return "n_a";
            case Subtype_Other:
                return "other";
            case Subtype_PatchPanel:
                return "patchpanel";
            case Subtype_PDU:
                return "pdu";
            case Subtype_RackController:
                return "rackcontroller";
            case Subtype_Router:
                return "router";
            case Subtype_Sensor:
                return "sensor";
            case Subtype_SensorGPIO:
                return "sensorgpio";
            case Subtype_Server:
                return "server";
            case Subtype_Sink:
                return "sink";
            case Subtype_Storage:
                return "storage";
            case Subtype_STS:
                return "sts";
            case Subtype_Switch:
                return "switch";
            case Subtype_UPS:
                return "ups";
            case Subtype_VM:
                return "vm";
            case Subtype_VMWareCluster:
                return "vmware.cluster";
            case Subtype_VMWareESXI:
                return "vmware.esxi";
            case Subtype_VMWareStandaloneESXI:
                return "vmware.standaloneesxi";
            case Subtype_VMWareTask:
                return "vmware.task";
            case Subtype_VMWareVApp:
                return "vmware.vapp";
            case Subtype_VMWareVCenter:
                return "vmware.vcenter";
            case Subtype_VMWareVM:
                return "vmware.vm";
            case Subtype_PCU:
                return "pcu";
            default:
                throw std::invalid_argument ("subtype id is not known value");
        }
    }

    BasicAsset::Subtype BasicAsset::stringToSubtype (const std::string & subtype) const
    {
        if (subtype == "appliance") {
            return Subtype_Appliance;
        } else if (subtype == "chassis") {
            return Subtype_Chassis;
        } else if (subtype == "citrix.pool") {
            return Subtype_CitrixPool;
        } else if (subtype == "citrix.task") {
            return Subtype_CitrixTask;
        } else if (subtype == "citrix.vapp") {
            return Subtype_CitrixVApp;
        } else if (subtype == "citrix.vm") {
            return Subtype_CitrixVM;
        } else if (subtype == "citrix.xenserver") {
            return Subtype_CitrixXenserver;
        } else if (subtype == "epdu") {
            return Subtype_EPDU;
        } else if (subtype == "feed") {
            return Subtype_Feed;
        } else if (subtype == "genset") {
            return Subtype_Genset;
        } else if (subtype == "gpo") {
            return Subtype_GPO;
        } else if (subtype == "hp.it.manager") {
            return Subtype_HPITManager;
        } else if (subtype == "hp.it.manager.service") {
            return Subtype_HPITManagerService;
        } else if (subtype == "hp.itrack") {
            return Subtype_HPITRack;
        } else if (subtype == "hp.it.server") {
            return Subtype_HPITServer;
        } else if (subtype == "ipminfra.server") {
            return Subtype_IPMInfraServer;
        } else if (subtype == "ipminfra.service") {
            return Subtype_IPMInfraService;
        } else if (subtype == "microsoft.cluster") {
            return Subtype_MicrosoftCluster;
        } else if (subtype == "microsoft.hyperv") {
            return Subtype_MicrosoftHyperV;
        } else if (subtype == "microsoft.server") {
            return Subtype_MicrosoftServer;
        } else if (subtype == "microsoft.task") {
            return Subtype_MicrosoftTask;
        } else if (subtype == "microsoft.virtualization.machine") {
            return Subtype_MicrosoftVirtualizationMachine;
        } else if (subtype == "microsoft.vm") {
            return Subtype_MicrosoftVM;
        } else if (subtype == "microsoft.windows.server") {
            return Subtype_MicrosoftWindowsServer;
        } else if (subtype == "netapp.cluster") {
            return Subtype_NetAppCluster;
        } else if (subtype == "netapp.node") {
            return Subtype_NetAppNode;
        } else if (subtype == "netapp.ontap.node") {
            return Subtype_NetAppOntapNode;
        } else if (subtype == "netapp.ontap.system") {
            return Subtype_NetAppOntapSystem;
        } else if (subtype == "netapp.server") {
            return Subtype_NetAppServer;
        } else if (subtype == "nutanix.cluster") {
            return Subtype_NutanixCluster;
        } else if (subtype == "nutanix.node") {
            return Subtype_NutanixNode;
        } else if (subtype == "nutanix.prism.gateway") {
            return Subtype_NutanixPrismGateway;
        } else if (subtype == "nutanix.virtualization.machine") {
            return Subtype_NutanixVirtualizationMachine;
        } else if (subtype == "n_a" || subtype == "N_A" || subtype.empty()) {
            return Subtype_N_A;
        } else if (subtype == "other") {
            return Subtype_Other;
        } else if (subtype == "patchpanel" || subtype == "patch panel") {
            return Subtype_PatchPanel;
        } else if (subtype == "pdu") {
            return Subtype_PDU;
        } else if (subtype == "rackcontroller" || subtype == "rack controller") {
            return Subtype_RackController;
        } else if (subtype == "router") {
            return Subtype_Router;
        } else if (subtype == "sensor") {
            return Subtype_Sensor;
        } else if (subtype == "sensorgpio") {
            return Subtype_SensorGPIO;
        } else if (subtype == "server") {
            return Subtype_Server;
        } else if (subtype == "sink") {
            return Subtype_Sink;
        } else if (subtype == "storage") {
            return Subtype_Storage;
        } else if (subtype == "sts") {
            return Subtype_STS;
        } else if (subtype == "switch") {
            return Subtype_Switch;
        } else if (subtype == "ups") {
            return Subtype_UPS;
        } else if (subtype == "vm") {
            return Subtype_VM;
        } else if (subtype == "vmware.cluster") {
            return Subtype_VMWareCluster;
        } else if (subtype == "vmware.esxi") {
            return Subtype_VMWareESXI;
        } else if (subtype == "vmware.standalone.esxi") {
            return Subtype_VMWareStandaloneESXI;
        } else if (subtype == "vmware.task") {
            return Subtype_VMWareTask;
        } else if (subtype == "vmware.vapp") {
            return Subtype_VMWareVApp;
        } else if (subtype == "vmware.vcenter") {
            return Subtype_VMWareVCenter;
        } else if (subtype == "vmware.vm") {
            return Subtype_VMWareVM;
        } else if (subtype == "pcu") {
            return Subtype_PCU;
        } else {
            throw std::invalid_argument ("subtype is not known value");
        }
    }

    std::string BasicAsset::statusToString (BasicAsset::Status status) const
    {
        switch (status) {
            case Status::Active:
                return "active";
            case Status::Nonactive:
                return "nonactive";
            default:
                throw std::invalid_argument ("status is not known value");
        }
    }

    BasicAsset::Status BasicAsset::stringToStatus (const std::string & status) const
    {
        if (status == "active") {
            return Status::Active;
        } else if (status == "nonactive") {
            return Status::Nonactive;
        } else {
            throw std::invalid_argument (TRANSLATE_ME ("status is not known value"));
        }
    }

    std::string BasicAsset::toJson() const
    {
        cxxtools::SerializationInfo rootSi;
        rootSi <<= *this;
        std::ostringstream assetJsonStream;
        cxxtools::JsonSerializer serializer (assetJsonStream);
        serializer.serialize (rootSi).finish();
        return assetJsonStream.str();
    }

    bool BasicAsset::isPowerAsset () const
    {
        Type type = type_subtype_.first;
        Subtype subtype = type_subtype_.second;
        return (type == Type_Device && (subtype == Subtype_EPDU || subtype == Subtype_Genset || subtype == Subtype_PDU || subtype == Subtype_STS || subtype == Subtype_UPS));
    }

    ExtendedAsset::ExtendedAsset ()
    { }

    ExtendedAsset::ExtendedAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, int priority)
            : BasicAsset (id, status, type, subtype), name_(name), parent_id_(parent_id), priority_(fty::convert<uint8_t>(priority))
    { }

    ExtendedAsset::ExtendedAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, const std::string & priority)
            : BasicAsset (id, status, type, subtype), name_(name), parent_id_(parent_id)
    {
        setPriority (priority);
    }

    ExtendedAsset::ExtendedAsset (fty_proto_t *msg): BasicAsset (msg)
    {
        name_ = fty_proto_ext_string (msg, "name", fty_proto_name (msg));
        parent_id_ = fty_proto_aux_string (msg, "parent_name.1", "");
        priority_ = fty::convert<uint8_t>(fty_proto_aux_number (msg, "priority", 5));
    }

    ExtendedAsset::ExtendedAsset (const cxxtools::SerializationInfo & si)
    {
        si >>= *this;
    }

    void ExtendedAsset::deserialize (const cxxtools::SerializationInfo & si)
    {
        BasicAsset::deserialize (si);
        si.getMember("name") >>= name_;

        if (!si.findMember ("priority"))
        {
            throw std::runtime_error("priority");
        }
        std::string priority_str;
        si.getMember("priority") >>= priority_str;
        if (priority_str.empty())
        {
            throw std::runtime_error("priority");
        }
        this->setPriority (priority_str);

        if (!si.findMember ("location"))
        {
            throw std::runtime_error("location");
        }
        si.getMember("location") >>= parent_id_;
    }

    void ExtendedAsset::serialize (cxxtools::SerializationInfo & si) const
    {
        BasicAsset::serialize (si);
        si.addMember("name") <<= name_;

        std::string priority_str = this->getPriorityString();
        si.addMember("priority") <<= priority_str;

        si.addMember("location") <<= parent_id_;
    }

    std::string ExtendedAsset::toJson() const
    {
        cxxtools::SerializationInfo rootSi;
        rootSi <<= *this;
        std::ostringstream assetJsonStream;
        cxxtools::JsonSerializer serializer (assetJsonStream);
        serializer.serialize (rootSi).finish();
        return assetJsonStream.str();
    }

    bool ExtendedAsset::operator == (const ExtendedAsset &asset) const
    {
        return BasicAsset::operator == (asset) && name_ == asset.name_ && parent_id_ == asset.parent_id_ &&
            priority_ == asset.priority_;
    }

    bool ExtendedAsset::operator!= (const ExtendedAsset &asset) const
    {
        return !(*this == asset);
    }

    // getters/setters
    std::string ExtendedAsset::getName () const
    {
        return name_;
    }

    std::string ExtendedAsset::getParentId () const
    {
        return parent_id_;
    }

    std::string ExtendedAsset::getPriorityString () const
    {
        return std::string ("P") + std::to_string (priority_);
    }

    int ExtendedAsset::getPriority () const
    {
        return priority_;
    }

    void ExtendedAsset::setName (const std::string & name)
    {
        name_ = name;
    }

    void ExtendedAsset::setParentId (const std::string & parent_id)
    {
        parent_id_ = parent_id;
    }

    void ExtendedAsset::setPriority (int priority)
    {
        priority_ = fty::convert<uint8_t>(priority);
    }

    void ExtendedAsset::setPriority (const std::string & priority)
    {
        priority_ = fty::convert<uint8_t>(priority);
    }

    FullAsset::FullAsset()
    { }

    FullAsset::FullAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, int priority, HashMap aux, HashMap ext)
            : ExtendedAsset (id, status, type, subtype, name, parent_id, priority), aux_(aux), ext_(ext)
    { }

    FullAsset::FullAsset (const std::string & id, const std::string & status, const std::string & type, const std::string & subtype, const std::string & name,
            const std::string & parent_id, const std::string & priority, HashMap aux, HashMap ext)
            : ExtendedAsset (id, status, type, subtype, name, parent_id, priority), aux_(aux), ext_(ext)
    { }

    FullAsset::FullAsset (fty_proto_t *msg): ExtendedAsset (msg)
    {
        // get our own copies since zhash_to_map doesn't do copying
        zhash_t *aux = fty_proto_aux (msg);
        zhash_t *ext = fty_proto_ext (msg);
        aux_ = MlmUtils::zhash_to_map (aux);
        ext_ = MlmUtils::zhash_to_map (ext);
    }

    FullAsset::FullAsset (const cxxtools::SerializationInfo & si)
    {
       si >>= *this;
    }

    void FullAsset::deserialize (const cxxtools::SerializationInfo & si)
    {
        ExtendedAsset::deserialize (si);

        if (si.findMember ("aux"))
        {
            const cxxtools::SerializationInfo auxSi = si.getMember("aux");
            for (const auto oneElement : auxSi)
            {
                auto key = oneElement.name();
                std::string value;
                oneElement >>= value;
                aux_[key] = value;
            }
        }

        if (si.findMember ("ext"))
        {
            const cxxtools::SerializationInfo extSi = si.getMember("ext");
            for (const auto oneElement : extSi)
            {
                auto key = oneElement.name();
                // ext from UI behaves as an object of objects with empty 1st level keys
                if (key.empty())
                {
                    for (const auto innerElement : oneElement)
                    {
                        auto innerKey = innerElement.name();
                        log_debug ("inner key = %s", innerKey.c_str ());
                        // only DB is interested in read_only attribute
                        if (innerKey != "read_only")
                        {
                            std::string value;
                            innerElement >>= value;
                            ext_[innerKey] = value;
                        }
                    }
                }
                else
                {
                    std::string value;
                    oneElement >>= value;
                    log_debug ("key = %s, value = %s", key.c_str (), value.c_str ());
                    ext_[key] = value;
                }
            }
        }
    }

    void FullAsset::serialize (cxxtools::SerializationInfo & si) const
    {
        ExtendedAsset::serialize (si);

        cxxtools::SerializationInfo &auxSi = si.addMember("aux");
        auxSi.setCategory (cxxtools::SerializationInfo::Object);
        for (const auto keyValue : aux_)
        {
            auto key = keyValue.first;
            auto value = keyValue.second;
            cxxtools::SerializationInfo &keyValueObject = auxSi.addMember (key);
            keyValueObject.setCategory (cxxtools::SerializationInfo::Object);
            keyValueObject.setValue (value);
        }

        cxxtools::SerializationInfo &extSi = si.addMember("ext");
        extSi.setCategory (cxxtools::SerializationInfo::Object);
        for (const auto keyValue : ext_)
        {
            auto key = keyValue.first;
            auto value = keyValue.second;
            cxxtools::SerializationInfo &keyValueObject = extSi.addMember (key);
            keyValueObject.setCategory (cxxtools::SerializationInfo::Object);
            keyValueObject.setValue (value);
        }
    }

    std::string FullAsset::toJson() const
    {
        cxxtools::SerializationInfo rootSi;
        rootSi <<= *this;
        std::ostringstream assetJsonStream;
        cxxtools::JsonSerializer serializer (assetJsonStream);
        serializer.serialize (rootSi).finish();
        return assetJsonStream.str();
    }

    bool FullAsset::operator == (const FullAsset &asset) const
    {
        return ExtendedAsset::operator == (asset) && aux_ == asset.aux_ && ext_ == asset.ext_;
    }

    bool FullAsset::operator!= (const FullAsset &asset) const
    {
        return !(*this == asset);
    }

    // getters/setters
    FullAsset::HashMap FullAsset::getAux () const
    {
        return aux_;
    }

    FullAsset::HashMap FullAsset::getExt () const
    {
        return ext_;
    }

    void FullAsset::setAux (FullAsset::HashMap aux)
    {
        aux_ = aux;
    }

    void FullAsset::setExt (FullAsset::HashMap ext)
    {
        ext_ = ext;
    }

    void FullAsset::setAuxItem (const std::string &key, const std::string &value)
    {
        aux_[key] = value;
    }

    void FullAsset::setExtItem (const std::string &key, const std::string &value)
    {
        ext_[key] = value;
    }

    std::string FullAsset::getAuxItem (const std::string &key) const
    {
        auto it = aux_.find (key);
        if (it != aux_.end ()) {
            return it->second;
        }
        return std::string ();
    }

    std::string FullAsset::getExtItem (const std::string &key) const
    {
        auto it = ext_.find (key);
        if (it != ext_.end ()) {
            return it->second;
        }
        return std::string ();
    }

    std::string FullAsset::getItem (const std::string &key) const
    {
        auto it = ext_.find (key);
        if (it != ext_.end ()) {
            return it->second;
        } else {
            auto it2 = aux_.find (key);
            if (it2 != aux_.end ()) {
                return it2->second;
            }
        }
        return std::string ();
    }

    std::unique_ptr<BasicAsset> getBasicAssetFromFtyProto (fty_proto_t *msg)
    {
        if (fty_proto_id (msg) != FTY_PROTO_ASSET)
            throw std::invalid_argument ("Wrong fty-proto type");
        return std::unique_ptr<BasicAsset>(new BasicAsset (
            fty_proto_name (msg),
            fty_proto_aux_string (msg, "status", "active"),
            fty_proto_aux_string (msg, "type", ""),
            fty_proto_aux_string (msg, "subtype", "")));
    }

    std::unique_ptr<ExtendedAsset> getExtendedAssetFromFtyProto (fty_proto_t *msg)
    {
        if (fty_proto_id (msg) != FTY_PROTO_ASSET)
            throw std::invalid_argument ("Wrong fty-proto type");
        return std::unique_ptr<ExtendedAsset>(new ExtendedAsset (
            fty_proto_name (msg),
            fty_proto_aux_string (msg, "status", "active"),
            fty_proto_aux_string (msg, "type", ""),
            fty_proto_aux_string (msg, "subtype", ""),
            fty_proto_ext_string (msg, "name", fty_proto_name (msg)),
            fty_proto_aux_string (msg, "parent_name.1", ""),
            fty::convert<int>(fty_proto_aux_number (msg, "priority", 5))));
    }

    std::unique_ptr<FullAsset> getFullAssetFromFtyProto (fty_proto_t *msg)
    {
        if (fty_proto_id (msg) != FTY_PROTO_ASSET)
            throw std::invalid_argument ("Wrong fty-proto type");

        zhash_t *aux = fty_proto_aux (msg);
        zhash_t *ext = fty_proto_ext (msg);
        return std::unique_ptr<FullAsset>(new FullAsset (
            fty_proto_name (msg),
            fty_proto_aux_string (msg, "status", "active"),
            fty_proto_aux_string (msg, "type", ""),
            fty_proto_aux_string (msg, "subtype", ""),
            fty_proto_ext_string (msg, "name", fty_proto_name (msg)),
            fty_proto_aux_string (msg, "parent_name.1", ""),
            fty::convert<uint8_t>(fty_proto_aux_number (msg, "priority", 5)),
            MlmUtils::zhash_to_map (aux),
            MlmUtils::zhash_to_map (ext))
            );
    }

} // end namespace

    void operator>>= (const cxxtools::SerializationInfo & si, fty::BasicAsset & asset)
    {
        asset.deserialize (si);
    }

    void operator>>= (const cxxtools::SerializationInfo & si, fty::ExtendedAsset & asset)
    {
        asset.deserialize (si);
    }

    void operator>>= (const cxxtools::SerializationInfo & si, fty::FullAsset & asset)
    {
        asset.deserialize (si);
    }

    void operator<<= (cxxtools::SerializationInfo & si, const fty::BasicAsset & asset)
    {
        asset.serialize (si);
    }

    void operator<<= (cxxtools::SerializationInfo & si, const fty::ExtendedAsset & asset)
    {
        asset.serialize (si);
    }

    void operator<<= (cxxtools::SerializationInfo & si, const fty::FullAsset & asset)
    {
        asset.serialize (si);
    }
//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

void
fty_common_asset_test (bool /* verbose */)
{
    printf (" * fty_common_asset: ");

    //  @selftest
    try {
        //fty::BasicAsset a; // this causes g++ error, as expected
        fty::BasicAsset b ("id-1", "active", "device", "rackcontroller");
        assert (b.getId () == "id-1");
        assert (b.getStatus () == fty::BasicAsset::Status::Active);
        assert (b.getType () == fty::BasicAsset::Type::Type_Device);
        assert (b.getSubtype () == fty::BasicAsset::Subtype::Subtype_RackController);
        assert (b.getStatusString () == "active");
        assert (b.getTypeString () == "device");
        assert (b.getSubtypeString () == "rackcontroller");
        b.setStatus ("nonactive");
        assert (b.getStatus () == fty::BasicAsset::Status::Nonactive);
        b.setType ("vm");
        assert (b.getType () == fty::BasicAsset::Type::Type_VM);
        b.setSubtype ("vmwarevm");
        assert (b.getSubtype () == fty::BasicAsset::Subtype::Subtype_VMWareVM);
        fty::BasicAsset bb (b);
        assert (b == bb);
        assert (bb.getId () == "id-1");
        assert (bb.getType () == fty::BasicAsset::Type::Type_VM);
        bb.setType ("device");
        assert (bb.getType () == fty::BasicAsset::Type::Type_Device);
        assert (b.getType () == fty::BasicAsset::Type::Type_VM);
        assert (b != bb);

        fty::BasicAsset bJson1 ("id-1", "active", "device", "rackcontroller");
        cxxtools::SerializationInfo rootSi;
        rootSi <<= bJson1;
        std::ostringstream assetJsonStream;
        cxxtools::JsonSerializer serializer (assetJsonStream);
        serializer.serialize (rootSi).finish();
        std::cerr << assetJsonStream.str () << std::endl;

        fty::BasicAsset bJson2;
        rootSi >>= bJson2;
        assert (bJson1.getId() == bJson2.getId());
        assert (bJson1.getStatusString() == bJson2.getStatusString());
        assert (bJson1.getTypeString() == bJson2.getTypeString());
        assert (bJson1.getSubtypeString() == bJson2.getSubtypeString());

    } catch (std::exception &e) {
        assert (false); // exception not expected
    }
    try {
        fty::BasicAsset c ("id-2", "invalid", "device", "rackcontroller");
        assert (false); // exception expected
    } catch (std::exception &e) {
        // exception is expected
    }
    try {
        fty::BasicAsset d ("id-3", "active", "invalid", "rackcontroller");
        assert (false); // exception expected
    } catch (std::exception &e) {
        // exception is expected
    }
    try {
        fty::BasicAsset e ("id-4", "active", "device", "invalid");
        assert (false); // exception expected
    } catch (std::exception &e) {
        // exception is expected
    }
    try {
        fty::ExtendedAsset f ("id-5", "active", "device", "rackcontroller", "MyRack", "id-1", 1);
        assert (f.getName () == "MyRack");
        assert (f.getParentId () == "id-1");
        assert (f.getPriority () == 1);
        assert (f.getPriorityString () == "P1");
        fty::ExtendedAsset g ("id-6", "active", "device", "rackcontroller", "MyRack", "parent-1", "P2");
        assert (f != g);
        assert (g.getPriority () == 2);
        assert (g.getPriorityString () == "P2");
        g.setName ("MyNewRack");
        assert (g.getName () == "MyNewRack");
        g.setParentId ("parent-2");
        assert (g.getParentId () == "parent-2");
        g.setPriority ("P3");
        assert (g.getPriority () == 3);
        g.setPriority (4);
        assert (g.getPriority () == 4);
        fty::ExtendedAsset gg (g);
        assert (g == gg);
        assert (gg.getId () == "id-6");
        assert (gg.getName () == "MyNewRack");
        gg.setName ("MyOldRack");
        assert (gg.getName () == "MyOldRack");
        assert (g.getName () == "MyNewRack");
        assert (g != gg);

        fty::ExtendedAsset fJson1 ("id-5", "active", "device", "rackcontroller", "MyRack", "id-1", 1);
        cxxtools::SerializationInfo rootSi;
        rootSi <<= fJson1;
        std::ostringstream assetJsonStream;
        cxxtools::JsonSerializer serializer (assetJsonStream);
        serializer.serialize (rootSi).finish ();
        std::cerr << assetJsonStream.str () << std::endl;

        fty::ExtendedAsset fJson2;
        rootSi >>= fJson2;
        assert (fJson1.getId() == fJson2.getId());
        assert (fJson1.getStatusString() == fJson2.getStatusString());
        assert (fJson1.getTypeString() == fJson2.getTypeString());
        assert (fJson1.getSubtypeString() == fJson2.getSubtypeString());
        assert (fJson1.getName() == fJson2.getName());
        assert (fJson1.getParentId() == fJson2.getParentId());
        assert (fJson1.getPriority() == fJson2.getPriority());

    } catch (std::exception &e) {
        assert (false); // exception not expected
    }
    try {
        fty::FullAsset h ("id-7", "active", "device", "rackcontroller", "MyRack", "id-1", 1, {{"aux1", "aval1"},
                {"aux2", "aval2"}}, {});
        assert (h.getAuxItem ("aux2") == "aval2");
        assert (h.getAuxItem ("aval3").empty ());
        assert (h.getExtItem ("eval1").empty ());
        h.setAuxItem ("aux4", "aval4");
        assert (h.getAuxItem ("aux4") == "aval4");
        h.setExtItem ("ext5", "eval5");
        assert (h.getExtItem ("ext5") == "eval5");
        h.setExt ({{"ext1", "eval1"}});
        assert (h.getExtItem ("ext1") == "eval1");
        assert (h.getExtItem ("ext5") != "eval5");
        assert (h.getItem ("aux2") == "aval2");
        assert (h.getItem ("ext1") == "eval1");
        assert (h.getItem ("notthere").empty ());
        fty::FullAsset hh (h);
        assert (h == hh);
        assert (hh.getExtItem ("ext1") == "eval1");
        assert (hh.getExtItem ("ext6").empty ());
        hh.setExtItem ("ext6", "eval6");
        assert (hh.getExtItem ("ext6") == "eval6");
        assert (h.getExtItem ("ext6").empty ());
        assert (h != hh);

        fty::FullAsset hJson1 ("id-7", "active", "device", "rackcontroller", "MyRack", "id-1", 1, {{"parent", "1"},{"parent_name.1","id-1"}}, {{"name","MyRack"}});
        cxxtools::SerializationInfo rootSi;
        rootSi <<= hJson1;
        std::ostringstream assetJsonStream;
        cxxtools::JsonSerializer serializer (assetJsonStream);
        serializer.serialize (rootSi).finish ();
        std::cerr << assetJsonStream.str () << std::endl;

        fty::FullAsset hJson2;
        rootSi >>= hJson2;

        assert (hJson1.getId() == hJson2.getId());
        assert (hJson1.getStatusString() == hJson2.getStatusString());
        assert (hJson1.getTypeString() == hJson2.getTypeString());
        assert (hJson1.getSubtypeString() == hJson2.getSubtypeString());
        assert (hJson1.getName() == hJson2.getName());
        assert (hJson1.getParentId() == hJson2.getParentId());
        assert (hJson1.getPriority() == hJson2.getPriority());

        assert (hJson1.getAuxItem("parent") == hJson2.getAuxItem("parent"));
        assert (hJson1.getAuxItem("parent_name.1") == hJson2.getAuxItem("parent_name.1"));
        assert (hJson1.getExtItem("name") == hJson2.getExtItem("name"));

    } catch (std::exception &e) {
        assert (false); // exception not expected
    }
    //  @end

    printf ("OK\n");
}
