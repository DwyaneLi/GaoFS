file(REMOVE_RECURSE
  "libmetadata_db.a"
  "libmetadata_db.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/metadata_db.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
