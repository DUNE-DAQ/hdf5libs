local moo = import "moo.jsonnet";
local ns = "dunedaq.hdf5libs.hdf5sourceidmaps";
local s = moo.oschema.schema(ns);

local types = {
    numeric_value : s.number("NumericValue", "u4", doc="Reasonably sized number"),

    path_string : s.string("PathString", doc="A string representing an HDF5 path"),

    source_id_path_pair : s.record("SourceIDPathPair", [
        s.field("subsys", self.numeric_value, 0, doc="SourceID subsystem"),
        s.field("id", self.numeric_value, 0, doc="SourceID ID"),
        s.field("path", self.path_string, "", doc="Path for the DataSet in the HDF5 file")
    ], doc="A single SourceID-to-HDF5-path map entry"),

    list_of_map_entries : s.sequence("MapEntryList", self.source_id_path_pair, doc="List of SourceID to HDF5 path map entries"),

    source_id_path_map : s.record("SourceIDPathMap", [
        s.field("source_id_path_map_version", self.numeric_value, 1, doc="Version of the encoding of the source-id-path map into JSON"),
        s.field("source_id_version", self.numeric_value, 1, doc="Version of the SourceID class that was/should be used"),
        s.field("map_entries", self.list_of_map_entries, doc="The list of entries in the map"),
    ], doc="Information that is needed to build up the map of SourceIDs to HDF5 DataSet paths"),
};

moo.oschema.sort_select(types, ns)
