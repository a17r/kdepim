configure_file(kaddressbook-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kaddressbook-version.h @ONLY)


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/xxport/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/printing/
    )

if(KF5Prison_FOUND)
    add_definitions(-DHAVE_PRISON)
endif()

add_subdirectory(domain)

add_subdirectory(icons)
add_subdirectory(printing)
add_subdirectory(kconf_update)

if(KF5Gpgmepp_FOUND)
    add_definitions(-DQGPGME_FOUND)
endif()


set(kaddressbook_printing_SRCS
    printing/detailled/detailledstyle.cpp
    printing/mike/mikesstyle.cpp
    printing/printingwizard.cpp
    printing/printprogress.cpp
    printing/printstyle.cpp
    printing/ringbinder/ringbinderstyle.cpp
    printing/compact/compactstyle.cpp
    printing/grantlee/grantleeprintstyle.cpp
    printing/stylepage.cpp
    )

ki18n_wrap_ui(kaddressbook_printing_SRCS
    printing/ringbinder/rbs_appearance.ui
    printing/detailled/ds_appearance.ui
    printing/compact/compactstyle.ui
    )


set(kaddressbook_xxport_SRCS
    xxport/csv/csv_xxport.cpp
    xxport/csv/csvimportdialog.cpp
    xxport/csv/dateparser.cpp
    xxport/csv/qcsvmodel.cpp
    xxport/csv/qcsvreader.cpp
    xxport/csv/templateselectiondialog.cpp
    xxport/ldif/ldif_xxport.cpp
    xxport/ldap/ldap_xxport.cpp
    xxport/gmx/gmx_xxport.cpp
    xxport/shared/xxport.cpp
    xxport/vcard/vcard_xxport.cpp
    xxport/xxportfactory.cpp
    xxport/xxportmanager.cpp
    xxport/vcard/vcardviewerdialog.cpp
    xxport/vcard/vcardexportselectionwidget.cpp
    )

set(kaddressbook_plugin_SRCS
    plugininterface/kaddressbookplugininterface.cpp
    )

set(kaddressbook_LIB_SRCS
    contactlist.cpp
    aboutdata.cpp
    categoryfilterproxymodel.cpp
    categoryselectwidget.cpp
    contactfields.cpp
    contactselectionwidget.cpp
    contactsorter.cpp
    contactswitcher.cpp
    globalcontactmodel.cpp
    mainwidget.cpp
    modelcolumnmanager.cpp
    widgets/quicksearchwidget.cpp
    kaddressbookmigrateapplication.cpp
    contactselectiondialog.cpp
    ${kaddressbook_printing_SRCS}
    ${kaddressbook_xxport_SRCS}
    ${kaddressbook_plugin_SRCS}
    )

ecm_qt_declare_logging_category(kaddressbook_LIB_SRCS HEADER kaddressbook_debug.h IDENTIFIER KADDRESSBOOK_LOG CATEGORY_NAME log_kaddressbook)

qt5_add_dbus_adaptor( kaddressbook_LIB_SRCS org.kde.kaddressbook.xml mainwidget.h MainWidget)

kconfig_add_kcfg_files(kaddressbook_LIB_SRCS settings/settings.kcfgc)
add_library(kaddressbookprivate ${kaddressbook_LIB_SRCS})
generate_export_header(kaddressbookprivate BASE_NAME kaddressbook)
target_link_libraries(kaddressbookprivate
    PUBLIC
    KF5::PimCommon
    PRIVATE
    KF5::Libkdepim
    KF5::KaddressbookGrantlee
    KF5::AkonadiCore
    KF5::Contacts
    KF5::AkonadiContact
    KF5::KCMUtils
    KF5::Codecs
    KF5::GrantleeTheme
    KF5::IconThemes
    KF5::Completion
    KF5::XmlGui
    KF5::JobWidgets
    KF5::KIOWidgets
    Qt5::PrintSupport
    KF5::AkonadiSearchDebug
    KF5::I18n
    )
if (WIN32)
    target_link_libraries(kaddressbookprivate KF5::Gpgmepp)
else()
    target_link_libraries(kaddressbookprivate PRIVATE KF5::Gpgmepp-pthread)
endif()
if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()

if(KF5Gpgmepp_FOUND)
    target_link_libraries(kaddressbookprivate PRIVATE KF5::QGpgme)
endif()
if(KF5Prison_FOUND)
    target_link_libraries(kaddressbookprivate PRIVATE KF5::Prison)
endif()

target_include_directories(kaddressbookprivate PUBLIC "$<BUILD_INTERFACE:${kaddressbook_SOURCE_DIR};${kaddressbook_BINARY_DIR}>")

set_target_properties(kaddressbookprivate PROPERTIES VERSION ${KDEPIM_LIB_VERSION} SOVERSION ${KDEPIM_LIB_SOVERSION})
install(TARGETS kaddressbookprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

########### next target ###############

set(kaddressbook_SRCS
    kaddressbook_debug.cpp
    main.cpp
    mainwindow.cpp
    )

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-kaddressbook.png")
ecm_add_app_icon(kaddressbook_SRCS ICONS ${ICONS_SRCS})

add_executable(kaddressbook ${kaddressbook_SRCS})

target_link_libraries(kaddressbook
    KF5::KontactInterface
    kaddressbookprivate
    KF5::Crash
    )

if(KF5Gpgmepp_FOUND)
    target_link_libraries(kaddressbook KF5::QGpgme)
endif()

install(TARGETS kaddressbook ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS data/org.kde.kaddressbook.desktop data/kaddressbook-importer.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES data/org.kde.kaddressbook.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES kaddressbookui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kaddressbook)

set(kaddressbook_PART_SRCS kaddressbookpart.cpp kaddressbook_debug.cpp)
add_library(kaddressbookpart MODULE ${kaddressbook_PART_SRCS})
target_link_libraries(kaddressbookpart  kaddressbookprivate KF5::Parts KF5::IconThemes)
install(TARGETS kaddressbookpart DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES data/kaddressbookpart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES
    viewertemplates/contact.html
    viewertemplates/contact_embedded.html
    viewertemplates/contact_row.html
    viewertemplates/contactgroup.html
    viewertemplates/contactgroup_embedded.html
    viewertemplates/contactgroup_additionalfieldrow.html
    viewertemplates/contactgroup_memberrow.html
    viewertemplates/theme.desktop
    DESTINATION ${KDE_INSTALL_DATADIR}/kaddressbook/viewertemplates/default/
    )

install(FILES data/kaddressbook_themes.knsrc DESTINATION ${KDE_INSTALL_CONFDIR} )

install(DIRECTORY printing/grantlee/themes/ DESTINATION ${KDE_INSTALL_DATADIR}/kaddressbook/printing/themes/ )
