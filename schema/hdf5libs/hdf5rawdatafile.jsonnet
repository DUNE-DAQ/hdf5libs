local moo = import "moo.jsonnet";
local ns = "dunedaq.hdf5libs.hdf5rawdatafile";
local s = moo.oschema.schema(ns);

local types = {
    size : s.number("Size", "u8", doc="A count of very many things"),

    count : s.number("Count", "i4", doc="A count of not too many things"),

    flag: s.boolean("Flag", doc="Parameter that can be used to enable or disable functionality"),

    geo_id_params : s.record("GeoID", [
           s.field("det_id", self.count, 0, doc="Detector ID"),
           s.field("crate_id", self.count, 0, doc="Crate ID"),
           s.field("slot_id", self.count, 0, doc="Slot ID"),
           s.field("stream_id", self.count, 0, doc="Stream ID") ],
        doc="Geographic ID structure"),

    src_geo_id_entry : s.record("SrcIDGeoIDEntry", [
        s.field("src_id", self.size, 0, doc="Source ID"),
        s.field("geo_id", self.geo_id_params, doc="Geo ID")
    ], doc="SourceID GeoID Map entry"),

    src_geo_id_map : s.sequence("SrcIDGeoIDMap", self.src_geo_id_entry, doc="SourceID to GeoID map" ),

};

moo.oschema.sort_select(types, ns)
