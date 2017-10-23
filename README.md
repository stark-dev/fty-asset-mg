# fty-asset

fty-asset is an agent managing information about assets.
Assets are either locations (datacenter, room, row, rack) or devices (RC, UPS, EPDU, etc.).

## How to build

To build fty-asset project run:

```bash
./autogen.sh
./configure
make
make check # to run self-test
```

## How to run

To run fty-asset project:

* from within the source tree, run:

```bash
./src/fty-asset
```

For the other options available, refer to the manual page of fty-asset

* from an installed base, using systemd, run:

```bash
systemctl start fty-asset
```

### Configuration file

Configuration file - fty-asset.cfg - is currently ignored.
Agent reads environment variable BIOS_ASSETS_REPEAT, which sets how often are all the assets republished on ASSETS stream.

## Architecture

### Overview

fty-asset is composed of 3 actors:

* asset-server: does general asset management
* asset-autoupdate: publishes inventory messages when available information about RCs changes
* asset-inventory: parses inventory messages and stores their content into DB

In addition to actors, there are 2 timers:

* asset server timer: runs every BIOS_ASSET_REPEATS seconds (by default once every hour) and triggers republish of all known assets on ASSETS stream
* autoupdate timer: runs once in a 5 minutes and triggers update of information about all RCs

## Protocols

### Published metrics

Agent doesn't publish any metrics.

### Published alerts

Agent doesn't publish any alerts.

### Mailbox requests

It is possible to request the fty-asset agent for:

* power topology chains,
* republishing assets,
* list of assets in given container,
* list of assets of given type/subtype,
* creation and updating of assets,
* user-friendly name of given asset,
* all the data available about given asset

#### Power topology

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* TOPOLOGY_POWER/'asset-iname'

where
* '/' indicates a multipart string message
* 'asset-iname' MUST be the asset iname
* subject of the message MUST be TOPOLOGY

The FTY-ASSET-AGENT peer MUST respond with one of the messages back to USER
peer using MAILBOX SEND.

* TOPOLOGY_POWER/'asset-iname'/OK/'D1'/.../'Dn'
* TOPOLOGY_POWER/'asset-iname'/ERROR/reason

where
* '/' indicates a multipart frame message
* 'asset-iname' MUST be the same as in request
* 'D1',...,'Dn' MUST be assets in the power topology of 'asset-iname'
* 'reason' is string detailing reason for error. Possible values are:

     ASSET_NOT_FOUND / INTERNAL_ERROR
* subject of the message MUST be TOPOLOGY

#### Republishing assets

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* 'message'

where:

* subject MUST be REPUBLISH

AND EITHER

* message MUST consist of string $all

OR

* message MUST be multipart string message, where every string is asset iname (message MAY be empty)

The FTY-ASSET-AGENT peer MUST respond with:

* IF message was $all OR empty:

    publication of fty-proto asset update message on the stream ASSETS for each asset in DB
* IF message was multipart string message

    publication of fty-proto asset update message on the stream ASSETS for each asset in request

#### Assets in given container

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* GET/'container-name'/'type-1'/.../'type-n'

where
* '/' indicates a multipart string message
* 'container-name' MAY be asset iname
* 'type-1'/.../'type-n' MAY be asset types/subtypes
* subject of the message MUST be "ASSETS_IN_CONTAINER"

The FTY-ASSET-AGENT peer MUST respond with one of the messages back to USER
peer using MAILBOX SEND.

* OK/'A1'/.../'An'
* ERROR/<reason>

where
* '/' indicates a multipart frame message
* IF 'container-name' in request was empty AND 'type-1/.../type-n' in request was empty,

    'A1'/.../'An' MUST be ALL the assets in the DB (MAY be empty)
* IF 'container-name' in request was empty AND 'type-1/.../type-n' in request was non-empty,

    'A1'/.../'An' MUST be ALL the assets in the DB of ANY of the specified types/subtypes (MAY be empty)
* IF 'container-name' in request was non-empty AND 'type-1/.../type-n' in request was empty,

    'A1'/.../'An' MUST be ALL assets which have 'container-name' as parent on any level (MAY be empty)
* IF 'container-name' in request was non-empty AND 'type-1/.../type-n' in request was non-empty,

    'A1'/.../'An' MUST be ALL assets which have 'container-name' as parent on any level of ANY of the given types or subtypes (MAY be empty)
* 'reason' is string detailing reason for error. Possible values are:

    ASSET_NOT_FOUND / BAD_COMMAND/INTERNAL_ERROR
* subject of the message MUST be "ASSETS_IN_CONTAINER"

#### Assets of given type/subtype

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* GET/'type-1'/.../'type-n'

where
* '/' indicates a multipart frame message
* 'type-1'/.../'type-n' MAY be asset types/subtypes
* subject of the message MUST be "ASSETS".

The FTY-ASSET-AGENT peer MUST respond with one of the messages back to USER
peer using MAILBOX SEND.

* OK/'A1'/.../'An'
* ERROR/<reason>

where
* '/' indicates a multipart frame message
* IF 'type-1/.../type-n' in request was empty,

   'A1'/.../'An' MUST be ALL assets in the DB
* IF 'type-1/.../type-n' in request was non-empty,

   'A1'/.../'An' MUST be ALL assets of ANY of the specified types/subtypes in the DB
* 'reason' is string detailing reason for error. Possible values are:

    ASSET_NOT_FOUND / BAD_COMMAND / MISSING_COMMAND / INTERNAL_ERROR
* subject of the message MUST be "ASSETS".

#### Creation and updating of assets

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* 'asset'

where
* 'asset' is fty-proto create OR update asset message
* subject of the message MUST be "ASSET_MANIPULATION".

The FTY-ASSET-AGENT peer MUST:
* insert provided data into DB
* publish UPDATE message for given asset on stream ASSETS
* respond with one of the messages back to USER
peer using MAILBOX SEND:

    * OK/'asset-iname'
    * ERROR/<reason>

where
* '/' indicates a multipart frame message
* 'asset-iname' MUST be iname of created or updated asset
* 'reason' is string detailing reason for error. Possible values are:

    OPERATION_NOT_IMPLEMENTED (MAY be empty)
* subject of the message MUST be "ASSET_MANIPULATION".

#### Getting user-friendly name for given asset

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* 'asset-iname'

where
* 'asset-iname' is internal name of an asset
* subject of the message MUST be "ENAME_FROM_INAME".

The FTY-ASSET-AGENT peer MUST respond with one of the messages back to USER
peer using MAILBOX SEND.

* OK/'asset-ename'
* ERROR/<reason>

where
* '/' indicates a multipart frame message
* 'asset-ename' is user-friendly name corresponding to 'asset-iname' from the request
* 'reason' is string detailing reason for error. Possible values are:

   MISSING_INAME/ASSET_NOT_FOUND
* subject of the message MUST be "ENAME_FROM_INAME".

#### Getting all the available asset data for given asset

The USER peer sends the following messages using MAILBOX SEND to
FTY-ASSET-AGENT ("asset-agent") peer:

* GET/'asset-iname'

where
* '/' indicates a multipart frame message
* 'asset-iname' is internal name of an asset
* subject of the message MUST be "ASSET_DETAIL".

The FTY-ASSET-AGENT peer MUST respond with one of the messages back to USER
peer using MAILBOX SEND.

* 'asset-message'
* ERROR/<reason>

where
* '/' indicates a multipart frame message
* 'asset-message' is fty-proto update message containing available data for given asset
* 'reason' is string detailing reason for error. Possible values are:

   BAD_COMMAND
* subject of the message MUST be "ASSET_DETAIL".

### Stream subscriptions

Agent is subscribed to ASSETS stream.

If it receives inventory message, asset-inventory actor extracts data from it and puts them into DB.
If it receives any other asset message, asset-server actor for all the children of this asset:

* updates power topology

* updates stored data

* publishes update message on ASSETS stream
