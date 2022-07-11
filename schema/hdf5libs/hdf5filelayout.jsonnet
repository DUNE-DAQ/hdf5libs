local moo = import "moo.jsonnet";
local ns = "dunedaq.hdf5libs.hdf5filelayout";
local s = moo.oschema.schema(ns);

local types = {
    size : s.number("Size", "u8", doc="A count of very many things"),

    count : s.number("Count", "i4", doc="A count of not too many things"),

    factor : s.number("Factor", "f4", doc="A float number of 4 bytes"),

    hdf_string : s.string("HDFString", doc="A string used in the hdf5 configuration"),

    flag: s.boolean("Flag", doc="Parameter that can be used to enable or disable functionality"),

    subdet_path_params : s.record("PathParams", [
           s.field("detector_group_type", self.hdf_string, "unspecified",
                   doc="The special keyword that identifies this entry (e.g. \"TPC\", \"PDS\", \"TPC_TP\", etc.)"),
           s.field("detector_group_name", self.hdf_string, "unspecified",
                   doc="The detector name that should be used inside the HDF5 file"),
           s.field("element_name_prefix", self.hdf_string, "Element",
                   doc="Prefix for the element name"),
           s.field("digits_for_element_number", self.count, 5,
                   doc="Number of digits to use for the element ID number inside the HDF5 file") ],
        doc="Parameters for the HDF5 Group and DataSet names"),

    list_of_path_params : s.sequence("PathParamList", self.subdet_path_params, doc="List of subdetector path parameters" ),

    hdf5_file_layout_params: s.record("FileLayoutParams", [
        s.field("record_name_prefix", self.hdf_string, "TriggerRecord",
                doc="Prefix for the record name"),
        s.field("digits_for_record_number", self.count, 6,
                doc="Number of digits to use for the record number in the record name inside the HDF5 file"),
        s.field("digits_for_sequence_number", self.count, 4,
                doc="Number of digits to use for the sequence number in the TriggerRecord name inside the HDF5 file"),
        s.field("record_header_dataset_name", self.hdf_string, "TriggerRecordHeader",
                doc="Dataset name for the record header"),
        s.field("path_param_list", self.list_of_path_params, doc=""),
    ], doc="Parameters for the layout of Groups and DataSets within the HDF5 file"),

};

moo.oschema.sort_select(types, ns)
