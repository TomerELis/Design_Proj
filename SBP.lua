-- Define the protocol
sbp_proto = Proto("sbp", "Sale Bidding Protocol")

-- Define fields for the SBP protocol
local f_version = ProtoField.uint8("sbp.version", "Version", base.HEX)
local f_type = ProtoField.uint8("sbp.type", "Type", base.HEX)
local f_length = ProtoField.uint16("sbp.length", "Length", base.DEC)
local f_reserved = ProtoField.uint8("sbp.reserved", "Reserved", base.HEX)
local f_client_id = ProtoField.uint16("sbp.client_id", "Client ID", base.DEC)
local f_sale_id = ProtoField.uint8("sbp.sale_id", "Sale ID", base.DEC)
local f_item_id = ProtoField.uint16("sbp.item_id", "Item ID", base.DEC)
local f_ip_address = ProtoField.ipv4("sbp.ip_address", "Multicast IP Address", base.NONE)
local f_port = ProtoField.uint16("sbp.port", "Multicast Port", base.DEC)
local f_price = ProtoField.uint32("sbp.price", "Price", base.DEC)

-- Add fields to the protocol
sbp_proto.fields = {
    f_version, f_type, f_length, f_reserved, 
    f_client_id, f_sale_id, f_item_id, 
    f_ip_address, f_port, f_price
}

-- Message types
local MESSAGE_TYPE_WELCOME = 0x01
local MESSAGE_TYPE_KEEP_ALIVE = 0x02
local MESSAGE_TYPE_SALE = 0x03
local MESSAGE_TYPE_ACK_SALE = 0x04
local MESSAGE_TYPE_SEND_ITEM = 0x05
local MESSAGE_TYPE_BINDING = 0x06
local MESSAGE_TYPE_ACK_BINDING = 0x07
local MESSAGE_TYPE_SEND_WINNER = 0x08

function sbp_proto.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "SBP"
    
    local subtree = tree:add(sbp_proto, buffer(), "SBP Protocol Data")

    -- Extract and add fields
    local version_and_type = buffer(0, 1):uint()
    local version = version_and_type >> 4  -- Upper 4 bits
    local type = version_and_type & 0x0F   -- Lower 4 bits

    subtree:add(f_version, buffer(0, 1), version)
    subtree:add(f_type, buffer(0, 1), type)
    subtree:add(f_length, buffer(1, 2))
    subtree:add(f_reserved, buffer(3, 1))

    local length = buffer(1, 2):uint()
    local offset = 4  -- Initial offset after the fixed header fields

    -- Apply colors based on message type
    if type == MESSAGE_TYPE_WELCOME then
        pinfo.cols.info:set("Welcome Message")
        subtree:add(f_client_id, buffer(offset, 2))
        offset = offset + 2
    elseif type == MESSAGE_TYPE_KEEP_ALIVE then
        pinfo.cols.info:set("Keep Alive Message")
        subtree:add(f_client_id, buffer(offset, 2))
        offset = offset + 2
    elseif type == MESSAGE_TYPE_SALE then
        pinfo.cols.info:set("Sale Message")
        subtree:add(f_sale_id, buffer(offset, 1))
        subtree:add(f_item_id, buffer(offset + 1, 2))
        offset = offset + 3
    elseif type == MESSAGE_TYPE_ACK_SALE then
        pinfo.cols.info:set("Ack Sale Message")
        subtree:add(f_ip_address, buffer(offset, 4))
        subtree:add(f_port, buffer(offset + 4, 2))
        offset = offset + 6
    elseif type == MESSAGE_TYPE_SEND_ITEM then
        pinfo.cols.info:set("Send Item Message")
        subtree:add(f_item_id, buffer(offset, 2))
        offset = offset + 2
    elseif type == MESSAGE_TYPE_BINDING then
        pinfo.cols.info:set("Binding Message")
        subtree:add(f_price, buffer(offset, 4))
        offset = offset + 4
    elseif type == MESSAGE_TYPE_ACK_BINDING then
        pinfo.cols.info:set("Ack Binding Message")
        subtree:add(f_client_id, buffer(offset, 2))
        offset = offset + 2
    elseif type == MESSAGE_TYPE_SEND_WINNER then
        pinfo.cols.info:set("Send Winner Message")
        subtree:add(f_client_id, buffer(offset, 2))
        subtree:add(f_price, buffer(offset + 2, 4))
        offset = offset + 6
    else
        subtree:add_expert_info(PI_PROTOCOL, PI_WARN, "Unknown SBP message type")
    end
end

-- Register dissector to a specific UDP port
udp_table = DissectorTable.get("udp.port")
udp_table:add(12345, sbp_proto)

