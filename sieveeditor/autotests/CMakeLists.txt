
set( sieveedit_serversievesettings_source  serversievesettingstest.cpp ../serversievesettings.cpp ../sieveeditor_debug.cpp)
ki18n_wrap_ui(sieveedit_serversievesettings_source
    ../ui/serversievesettings.ui
)

add_executable( serversievesettingstest  ${sieveedit_serversievesettings_source})
add_test(serversievesettingstest serversievesettingstest)
ecm_mark_as_test(serversievesettingstest)
target_link_libraries( serversievesettingstest Qt5::Test Qt5::Widgets KF5::MailTransport)

#REACTIVATE
if (false)
set( sieveeditorutiltest_source sieveeditorutiltest.cpp ../sieveeditorutil.cpp ../sieveserversettings.cpp)
add_executable( sieveeditorutiltest ${sieveeditorutiltest_source})
add_test(sieveeditorutiltest sieveeditorutiltest)
ecm_mark_as_test(sieveeditorutiltest)
target_link_libraries( sieveeditorutiltest Qt5::Test )
endif()