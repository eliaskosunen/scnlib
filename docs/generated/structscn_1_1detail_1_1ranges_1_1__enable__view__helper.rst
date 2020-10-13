.. _exhale_struct_structscn_1_1detail_1_1ranges_1_1__enable__view__helper:

Template Struct _enable_view_helper
===================================

- Defined in :ref:`file_include_scn_detail_ranges.h`


Inheritance Relationships
-------------------------

Base Type
*********

- ``public std::conditional::type< std::is_base_of< view_base, T >::value, std::true_type, std::conditional< _is_std_non_view< T >::value, std::false_type, std::conditional< range< T >::value &&range< const T >::value, std::is_same< range_reference_t< T >, range_reference_t< const T > >, std::true_type >::type >::type >``


Struct Documentation
--------------------


.. doxygenstruct:: scn::detail::ranges::_enable_view_helper
   :members:
   :protected-members:
   :undoc-members: