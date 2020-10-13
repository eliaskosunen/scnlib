.. _exhale_struct_structscn_1_1detail_1_1ranges_1_1__incrementable__traits__helper_3_01_t_00_01typename_01std_1_1e0ad534867afa11906226fac2ec0f08aa:

Template Struct _incrementable_traits_helper< T, typename std::enable_if< !std::is_pointer< T >::value &&!_has_member_difference_type< T >::value &&std::is_integral< decltype(std::declval< const T & >() - std::declval< const T & >())>::value >::type >
===========================================================================================================================================================================================================================================================

- Defined in :ref:`file_include_scn_detail_ranges.h`


Inheritance Relationships
-------------------------

Base Type
*********

- ``public scn::detail::ranges::_with_difference_type< std::make_signed< decltype(std::declval< T >() - std::declval< T >())>::type >`` (:ref:`exhale_struct_structscn_1_1detail_1_1ranges_1_1__with__difference__type`)


Struct Documentation
--------------------


.. doxygenstruct:: scn::detail::ranges::_incrementable_traits_helper< T, typename std::enable_if< !std::is_pointer< T >::value &&!_has_member_difference_type< T >::value &&std::is_integral< decltype(std::declval< const T & >() - std::declval< const T & >())>::value >::type >
   :members:
   :protected-members:
   :undoc-members: