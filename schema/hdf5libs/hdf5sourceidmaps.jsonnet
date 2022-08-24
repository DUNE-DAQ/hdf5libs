local moo = import "moo.jsonnet";
local ns = "dunedaq.hdf5libs.hdf5sourceidmaps";
local s = moo.oschema.schema(ns);

local types = {
    numeric_value : s.number("NumericValue", "u4", doc="Reasonably sized number"),

    path_string : s.string("PathString", doc="A string representing an HDF5 path"),

    geo_id_value : s.number("GeoIDValue", "u8", doc="64-bit number"),

    source_id : s.record("SourceID", [
        s.field("subsys", self.numeric_value, 0, doc="SourceID subsystem"),
        s.field("id", self.numeric_value, 0, doc="SourceID ID"),
        s.field("source_id_version", self.numeric_value, 1, doc="Version of the SourceID class that was/should be used")
    ], doc="A single SourceID"),

    # -------------------- #

    source_id_path_pair : s.record("SourceIDPathPair", [
        s.field("subsys", self.numeric_value, 0, doc="SourceID subsystem"),
        s.field("id", self.numeric_value, 0, doc="SourceID ID"),
        s.field("path", self.path_string, "", doc="Path for the DataSet in the HDF5 file")
    ], doc="A single SourceID-to-HDF5-path map entry"),

    list_of_path_map_entries : s.sequence("PathMapEntryList", self.source_id_path_pair, doc="List of SourceID to HDF5 path map entries"),

    source_id_path_map : s.record("SourceIDPathMap", [
        s.field("source_id_version", self.numeric_value, 1, doc="Version of the SourceID class that was/should be used"),
        s.field("map_entries", self.list_of_path_map_entries, doc="The list of entries in the map"),
    ], doc="Information that is needed to build up the map of SourceIDs to HDF5 DataSet paths"),

    # -------------------- #

    list_of_geo_ids : s.sequence("GeoIDList", self.geo_id_value, doc="List of GeoIDs"),

    source_id_geo_id_pair : s.record("SourceIDGeoIDPair", [
        s.field("subsys", self.numeric_value, 0, doc="SourceID subsystem"),
        s.field("id", self.numeric_value, 0, doc="SourceID ID"),
        s.field("geoids", self.list_of_geo_ids, doc="List of GeoIDs contained within the SourceID")
    ], doc="A single SourceID-to-HDF5-path map entry"),

    list_of_geo_id_map_entries : s.sequence("GeoIDMapEntryList", self.source_id_geo_id_pair, doc="List of SourceID to GeoID map entries"),

    source_id_geo_id_map : s.record("SourceIDGeoIDMap", [
        s.field("source_id_version", self.numeric_value, 1, doc="Version of the SourceID class that was/should be used"),
        s.field("map_entries", self.list_of_geo_id_map_entries, doc="The list of entries in the map"),
    ], doc="Information that is needed to build up the map of SourceIDs to GeoIDs"),

    # -------------------- #

    list_of_source_ids : s.sequence("SourceIDList", self.source_id, doc="List of SourceIDs"),

    fragment_type_source_id_pair : s.record("FragmentTypeSourceIDPair", [
        s.field("fragment_type", self.numeric_value, 0, doc="FragmentType"),
        s.field("sourceids", self.list_of_source_ids, doc="List of SourceIDs for a given FragmentType")
    ], doc="A single FragmentType-to-SourceID map entry"),

    list_of_fragment_type_map_entries : s.sequence("FragmentTypeMapEntryList", self.fragment_type_source_id_pair, doc="List of FragmentType to SourceID map entries"),

    fragment_type_source_id_map : s.record("FragmentTypeSourceIDMap", [
        s.field("source_id_version", self.numeric_value, 1, doc="Version of the SourceID class that was/should be used"),
        s.field("map_entries", self.list_of_fragment_type_map_entries, doc="The list of entries in the map"),
    ], doc="Information that is needed to build up the map of FragmentTypes to SourceIDs"),

    # -------------------- #

    subdetector_source_id_pair : s.record("SubdetectorSourceIDPair", [
        s.field("subdetector", self.numeric_value, 0, doc="DetID::Subdetector"),
        s.field("sourceids", self.list_of_source_ids, doc="List of SourceIDs for a given Subdetector")
    ], doc="A single Subdetector-to-SourceID map entry"),

    list_of_subdetector_map_entries : s.sequence("SubdetectorMapEntryList", self.subdetector_source_id_pair, doc="List of Subdetector to SourceID map entries"),

    subdetector_source_id_map : s.record("SubdetectorSourceIDMap", [
        s.field("source_id_version", self.numeric_value, 1, doc="Version of the SourceID class that was/should be used"),
        s.field("map_entries", self.list_of_subdetector_map_entries, doc="The list of entries in the map"),
    ], doc="Information that is needed to build up the map of Subdetectors to SourceIDs"),
};

moo.oschema.sort_select(types, ns)
