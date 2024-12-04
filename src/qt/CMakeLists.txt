include(CheckCXXSourceCompiles)

# find_package(Protobuf REQUIRED)
find_package(Qt5 COMPONENTS Widgets LinguistTools Network Core DBus QUIET)
if(NOT TARGET Qt5::Widgets)
    message(FATAL_ERROR "Failed to find Widgets for qt5")
endif()
if(NOT TARGET Qt5::lrelease)
    message(FATAL_ERROR "Failed to find LinguistTools for qt5")
endif()
if(NOT TARGET Qt5::Network)
    message(FATAL_ERROR "Failed to find Network for qt5")
endif()
if(NOT TARGET Qt5::Core)
    message(FATAL_ERROR "Failed to find Core for qt5")
endif()

option(WITHOUT_QR_ENCODE "Disable qt qrencode support. (Default is OFF if libqrencode found)" OFF)
if (NOT WITHOUT_QR_ENCODE)
    find_package(QREncode)
    if (NOT QREncode_FOUND)
        message(WARNING "QT is going to be built without qrencode support because libqrencode was not found. Use -DWITHOUT_QR_ENCODE=ON to suppress this warning")
    endif()
endif()

# This force cmake to automatically generate all moc files. See CMake documentation
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QT_TS
        locale/pocketcoin_af.ts
        locale/pocketcoin_am.ts
        locale/pocketcoin_ar.ts
        locale/pocketcoin_az.ts
        locale/pocketcoin_be.ts
        locale/pocketcoin_bg.ts
        locale/pocketcoin_bn.ts
        locale/pocketcoin_bs.ts
        locale/pocketcoin_ca.ts
        locale/pocketcoin_cs.ts
        locale/pocketcoin_cy.ts
        locale/pocketcoin_da.ts
        locale/pocketcoin_de.ts
        locale/pocketcoin_el.ts
        locale/pocketcoin_en.ts
        locale/pocketcoin_en_GB.ts
        locale/pocketcoin_eo.ts
        locale/pocketcoin_es.ts
        locale/pocketcoin_es_CL.ts
        locale/pocketcoin_es_CO.ts
        locale/pocketcoin_es_DO.ts
        locale/pocketcoin_es_MX.ts
        locale/pocketcoin_es_VE.ts
        locale/pocketcoin_et.ts
        locale/pocketcoin_eu.ts
        locale/pocketcoin_fa.ts
        locale/pocketcoin_fi.ts
        locale/pocketcoin_fil.ts
        locale/pocketcoin_fr.ts
        locale/pocketcoin_ga.ts
        locale/pocketcoin_gl.ts
        locale/pocketcoin_gl_ES.ts
        locale/pocketcoin_gu.ts
        locale/pocketcoin_he.ts
        locale/pocketcoin_hi.ts
        locale/pocketcoin_hr.ts
        locale/pocketcoin_hu.ts
        locale/pocketcoin_id.ts
        locale/pocketcoin_is.ts
        locale/pocketcoin_it.ts
        locale/pocketcoin_ja.ts
        locale/pocketcoin_ka.ts
        locale/pocketcoin_kk.ts
        locale/pocketcoin_kl.ts
        locale/pocketcoin_km.ts
        locale/pocketcoin_ko.ts
        locale/pocketcoin_ku_IQ.ts
        locale/pocketcoin_ky.ts
        locale/pocketcoin_la.ts
        locale/pocketcoin_lt.ts
        locale/pocketcoin_lv.ts
        locale/pocketcoin_mk.ts
        locale/pocketcoin_ml.ts
        locale/pocketcoin_mn.ts
        locale/pocketcoin_mr_IN.ts
        locale/pocketcoin_ms.ts
        locale/pocketcoin_my.ts
        locale/pocketcoin_nb.ts
        locale/pocketcoin_ne.ts
        locale/pocketcoin_nl.ts
        locale/pocketcoin_no.ts
        locale/pocketcoin_pam.ts
        locale/pocketcoin_pl.ts
        locale/pocketcoin_pt.ts
        locale/pocketcoin_pt_BR.ts
        locale/pocketcoin_ro.ts
        locale/pocketcoin_ru.ts
        locale/pocketcoin_si.ts
        locale/pocketcoin_sk.ts
        locale/pocketcoin_sl.ts
        locale/pocketcoin_sn.ts
        locale/pocketcoin_sq.ts
        locale/pocketcoin_sr.ts
        locale/pocketcoin_sr@latin.ts
        locale/pocketcoin_sv.ts
        locale/pocketcoin_szl.ts
        locale/pocketcoin_ta.ts
        locale/pocketcoin_te.ts
        locale/pocketcoin_th.ts
        locale/pocketcoin_tr.ts
        locale/pocketcoin_ug.ts
        locale/pocketcoin_uk.ts
        locale/pocketcoin_ur.ts
        locale/pocketcoin_uz@Cyrl.ts
        locale/pocketcoin_uz@Latn.ts
        locale/pocketcoin_vi.ts
        locale/pocketcoin_yo.ts
        locale/pocketcoin_zh-Hans.ts
        locale/pocketcoin_zh.ts
        locale/pocketcoin_zh_CN.ts
        locale/pocketcoin_zh_HK.ts
        locale/pocketcoin_zh_TW.ts
        locale/pocketcoin_zu.ts)

# Specify output path for generated qm files
foreach(QT_TS_FILE ${QT_TS})
    get_filename_component(QT_TS_CURRENT_FILE_DIR ${QT_TS_FILE} DIRECTORY)
    list(APPEND QT_TS_DIRS ${QT_TS_CURRENT_FILE_DIR})
endforeach()
list(REMOVE_DUPLICATES QT_TS_DIRS)
list(LENGTH QT_TS_DIRS QT_TS_DIRS_LENGTH)
if(QT_TS_DIRS_LENGTH GREATER 1)
    message(FATAL_ERROR "More than one subdirectory found for ts resources. Consider dividing them to different source lists")
endif()
list(GET QT_TS_DIRS 0 QT_TS_OUTPUT_PATH)
if(QT_TS_OUTPUT_PATH)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${QT_TS_OUTPUT_PATH}")
endif()
set_source_files_properties(${QT_TS} PROPERTIES OUTPUT_LOCATION ${QT_TS_OUTPUT_PATH})

#set(POCKETCOIN_MM
#        macdockiconhandler.mm
#        macnotificationhandler.mm
#        macos_appnap.mm)

# set(QT_QRC_CPP qrc_pocketcoin.cpp)
set(QT_QRC pocketcoin.qrc)
set(QT_QRC_LOCALE_CPP qrc_pocketcoin_locale.cpp)
set(QT_QRC_LOCALE pocketcoin_locale.qrc)

# Protobuf files generating
# set(PROTOBUF_PROTO paymentrequest.proto)
# protobuf_generate_cpp(PROTOBUF_CC PROTOBUF_H ${PROTOBUF_PROTO})
# add_custom_target(protobuf_generated ALL DEPENDS ${PROTOBUF_CC} ${PROTOBUF_H})

set(POCKETCOIN_QT_H
        addressbookpage.h
        addresstablemodel.h
        askpassphrasedialog.h
        bantablemodel.h
        pocketcoinaddressvalidator.h
        pocketcoinamountfield.h
        pocketcoin.h
        pocketcoingui.h
        pocketcoinunits.h
        clientmodel.h
        coincontroldialog.h
        coincontroltreewidget.h
        createwalletdialog.h
        csvmodelwriter.h
        editaddressdialog.h
        updatenotificationdialog.h
        guiconstants.h
        guiutil.h
        intro.h
        $<$<PLATFORM_ID:Darwin>:macdockiconhandler.h>
        $<$<PLATFORM_ID:Darwin>:macdockiconhandler.mm>
        $<$<PLATFORM_ID:Darwin>:macnotificationhandler.h>
        $<$<PLATFORM_ID:Darwin>:macnotificationhandler.mm>
        $<$<PLATFORM_ID:Darwin>:macos_appnap.h>
        $<$<PLATFORM_ID:Darwin>:macos_appnap.mm>
        modaloverlay.h
        networkstyle.h
        notificator.h
        openuridialog.h
        optionsdialog.h
        optionsmodel.h
        overviewpage.h
        paymentserver.h
        peertablemodel.h
        platformstyle.h
        psbtoperationsdialog.h
        qrimagewidget.h
        qvalidatedlineedit.h
        qvaluecombobox.h
        receivecoinsdialog.h
        receiverequestdialog.h
        recentrequeststablemodel.h
        rpcconsole.h
        sendcoinsdialog.h
        sendcoinsentry.h
        sendcoinsrecipient.h
        signverifymessagedialog.h
        splashscreen.h
        trafficgraphwidget.h
        transactiondesc.h
        transactiondescdialog.h
        transactionfilterproxy.h
        transactionoverviewwidget.h
        transactionrecord.h
        transactiontablemodel.h
        transactionview.h
        utilitydialog.h
        walletcontroller.h
        walletframe.h
        walletmodel.h
        walletmodeltransaction.h
        walletview.h
        $<$<PLATFORM_ID:Windows>:winshutdownmonitor.h>
)
#if(WIN32)
#    list(APPEND POCKETCOIN_QT_H winshutdownmonitor.h)
#endif()
#if(APPLE)
#    list(APPEND POCKETCOIN_QT_H macdockiconhandler.h macnotificationhandler.h macos_appnap.h)
#endif()

set(RES_ICONS
        res/icons/add.png
        res/icons/address-book.png
        res/icons/pocketcoin.ico
        res/icons/pocketcoin_testnet.ico
        res/icons/pocketcoin.png
        res/icons/chevron.png
        res/icons/clock1.png
        res/icons/clock2.png
        res/icons/clock3.png
        res/icons/clock4.png
        res/icons/clock5.png
        res/icons/connect0.png
        res/icons/connect1.png
        res/icons/connect2.png
        res/icons/connect3.png
        res/icons/connect4.png
        res/icons/edit.png
        res/icons/editcopy.png
        res/icons/editpaste.png
        res/icons/export.png
        res/icons/eye.png
        res/icons/eye_minus.png
        res/icons/eye_plus.png
        res/icons/fontbigger.png
        res/icons/fontsmaller.png
        res/icons/hd_disabled.png
        res/icons/hd_enabled.png
        res/icons/history.png
        res/icons/lock_closed.png
        res/icons/lock_open.png
        res/icons/network_disabled.png
        res/icons/overview.png
        res/icons/proxy.png
        res/icons/receive.png
        res/icons/remove.png
        res/icons/send.png
        res/icons/synced.png
        res/icons/transaction0.png
        res/icons/transaction2.png
        res/icons/transaction_abandoned.png
        res/icons/transaction_conflicted.png
        res/icons/tx_inout.png
        res/icons/tx_input.png
        res/icons/tx_output.png
        res/icons/tx_mined.png
        res/icons/warning.png)

set(POCKETCOIN_QT_BASE_CPP
        bantablemodel.cpp
        pocketcoin.cpp
        pocketcoinaddressvalidator.cpp
        pocketcoinamountfield.cpp
        pocketcoingui.cpp
        pocketcoinunits.cpp
        clientmodel.cpp
        csvmodelwriter.cpp
        guiutil.cpp
        intro.cpp
        modaloverlay.cpp
        networkstyle.cpp
        notificator.cpp
        optionsdialog.cpp
        optionsmodel.cpp
        peertablemodel.cpp
        platformstyle.cpp
        qvalidatedlineedit.cpp
        qvaluecombobox.cpp
        rpcconsole.cpp
        splashscreen.cpp
        trafficgraphwidget.cpp
        utilitydialog.cpp
        $<$<PLATFORM_ID:Windows>:winshutdownmonitor.cpp>
)

#set(POCKETCOIN_QT_WINDOWS_CPP winshutdownmonitor.cpp)

set(POCKETCOIN_QT_WALLET_CPP
        addressbookpage.cpp
        addresstablemodel.cpp
        askpassphrasedialog.cpp
        coincontroldialog.cpp
        coincontroltreewidget.cpp
        createwalletdialog.cpp
        editaddressdialog.cpp
        updatenotificationdialog.cpp
        openuridialog.cpp
        overviewpage.cpp
        paymentserver.cpp
        psbtoperationsdialog.cpp
        qrimagewidget.cpp
        receivecoinsdialog.cpp
        receiverequestdialog.cpp
        recentrequeststablemodel.cpp
        sendcoinsdialog.cpp
        sendcoinsentry.cpp
        signverifymessagedialog.cpp
        transactiondesc.cpp
        transactiondescdialog.cpp
        transactionfilterproxy.cpp
        transactionrecord.cpp
        transactiontablemodel.cpp
        transactionview.cpp
        walletcontroller.cpp
        walletframe.cpp
        walletmodel.cpp
        walletmodeltransaction.cpp
        walletview.cpp)

set(POCKETCOIN_QT_CPP ${POCKETCOIN_QT_BASE_CPP})
#if(WIN32)
#    list(APPEND POCKETCOIN_QT_CPP ${POCKETCOIN_QT_WINDOWS_CPP})
#endif()
if(NOT DISABLE_WALLET)
    list(APPEND POCKETCOIN_QT_CPP ${POCKETCOIN_QT_WALLET_CPP})
endif()

set(RES_ANUMATIONS res/animations/spinner-000.png
        res/animations/spinner-001.png
        res/animations/spinner-002.png
        res/animations/spinner-003.png
        res/animations/spinner-004.png
        res/animations/spinner-005.png
        res/animations/spinner-006.png
        res/animations/spinner-007.png
        res/animations/spinner-008.png
        res/animations/spinner-009.png
        res/animations/spinner-010.png
        res/animations/spinner-011.png
        res/animations/spinner-012.png
        res/animations/spinner-013.png
        res/animations/spinner-014.png
        res/animations/spinner-015.png
        res/animations/spinner-016.png
        res/animations/spinner-017.png
        res/animations/spinner-018.png
        res/animations/spinner-019.png
        res/animations/spinner-020.png
        res/animations/spinner-021.png
        res/animations/spinner-022.png
        res/animations/spinner-023.png
        res/animations/spinner-024.png
        res/animations/spinner-025.png
        res/animations/spinner-026.png
        res/animations/spinner-027.png
        res/animations/spinner-028.png
        res/animations/spinner-029.png
        res/animations/spinner-030.png
        res/animations/spinner-031.png
        res/animations/spinner-032.png
        res/animations/spinner-033.png
        res/animations/spinner-034.png
        res/animations/spinner-035.png)

set(POCKETCOIN_RC res/pocketcoin-qt-res.rc)

# Compiling ts files
qt5_add_translation(QT_QM_COMPILED ${QT_TS} OPTIONS)
configure_file(pocketcoin_locale.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
add_custom_target(translations_available ALL DEPENDS ${QT_QM_COMPILED})

# Compiling resources
qt5_add_resources(QT_QRC_COMPILED ${QT_QRC} OPTIONS -name pocketcoin)
qt5_add_resources(QT_QRC_LOCALE_COMPILED ${CMAKE_CURRENT_BINARY_DIR}/pocketcoin_locale.qrc OPTIONS -name pocketcoin_locale)
add_custom_target(resources ALL DEPENDS ${QT_QRC_COMPILED})
add_custom_target(resources_locale ALL DEPENDS ${QT_QRC_LOCALE_COMPILED})
add_dependencies(resources_locale translations_available)

# Process .ui files and create custom target "ui_done"
add_subdirectory(forms)

add_library(qt_libpocketcoinqt
        ${POCKETCOIN_QT_CPP}
        ${POCKETCOIN_QT_H}
        ${CMAKE_CURRENT_BINARY_DIR}/pocketcoin_locale.qrc # TODO hack to match relative paths in this file to generated resources
        ${QT_QRC_COMPILED}
        ${QT_QRC_LOCALE_COMPILED}
        ${RES_ICONS}
        ${RES_IMAGES}
        ${RES_ANIMATIONS}
        # ${PROTOBUF_CC}
        # ${PROTOBUF_H}
        )

target_compile_definitions(qt_libpocketcoinqt PUBLIC QT_NO_KEYWORDS)
target_link_libraries(qt_libpocketcoinqt PRIVATE Boost::filesystem OpenSSL::Crypto Oracle::BerkeleyDB leveldb ${POCKETCOIN_SERVER} ${POCKETCOIN_COMMON})
target_link_libraries(qt_libpocketcoinqt PUBLIC Qt5::Core Qt5::Widgets Qt5::Network)
set_target_properties(qt_libpocketcoinqt PROPERTIES PREFIX "") # Remove "lib" prefix because we already have it in library name
# Specify dependencies that force processing resources and ui files before this target
add_dependencies(qt_libpocketcoinqt resources resources_locale ui_done)
# TODO Add checks for this or allow use config as it is in autotools
#target_compile_definitions(qt_libpocketcoinqt PRIVATE HAVE_DECL_BSWAP_16=1 HAVE_DECL_BSWAP_32=1 HAVE_DECL_BSWAP_64=1 HAVE_DECL_EVP_MD_CTX_NEW=1)
# Include directory "${CMAKE_CURRENT_BINARY_DIR}/.." because generated files (resources, qm files, processed ui, etc) are
# located in build tree and are included by other qt files by using "#include <qt/filename>"
target_include_directories(qt_libpocketcoinqt PRIVATE ${OPENSSL_INCLUDE_DIR} ${Protobuf_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)
if(QREncode_FOUND)
    target_link_libraries(qt_libpocketcoinqt PRIVATE QREncode::qrencode)
    target_include_directories(qt_libpocketcoinqt PRIVATE ${QREncode_INCLUDE_DIRS})
    target_compile_definitions(qt_libpocketcoinqt PRIVATE USE_QRCODE=1)
endif()
if(TARGET Qt5::DBus)
    target_link_libraries(qt_libpocketcoinqt PRIVATE Qt5::DBus)
    target_compile_definitions(qt_libpocketcoinqt PRIVATE USE_DBUS=1)
endif()

# Checks
set(CMAKE_REQUIRED_LIBRARIES_BACKUP ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES Qt5::Core Qt5::Widgets)
check_cxx_source_compiles(" #include <QtCore/qconfig.h>
                            #ifndef QT_VERSION OR QT_VERSION_STR
                            #  include <QtCore/qglobal.h>
                            #endif
                            int main(){
                            #if !defined(QT_STATIC)
                                choke
                            #endif
                            return 0;
                            }" _POCKETCOIN_QT_STATIC)
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_BACKUP})
if(_POCKETCOIN_QT_STATIC)
    if(NOT TARGET Qt5::QMinimalIntegrationPlugin)
        message(FATAL_ERROR "Failed to find Qt static module: QMinimalIntegrationPlugin")
    endif()
    qt5_import_plugins(qt_libpocketcoinqt INCLUDE Qt5::QMinimalIntegrationPlugin)
    target_compile_definitions(qt_libpocketcoinqt PRIVATE QT_QPA_PLATFORM_MINIMAL=1)
    if(WIN32)
        if(NOT TARGET Qt5::QWindowsIntegrationPlugin)
            message(FATAL_ERROR "Failed to find Qt static module: QWindowsIntegrationPlugin")
        endif()
        qt5_import_plugins(qt_libpocketcoinqt INCLUDE Qt5::QWindowsIntegrationPlugin)
        target_compile_definitions(qt_libpocketcoinqt PRIVATE QT_QPA_PLATFORM_WINDOWS=1)
    elseif(APPLE)
        if(NOT TARGET Qt5::QCocoaIntegrationPlugin)
            message(FATAL_ERROR "Failed to find Qt static module: QCocoaIntegrationPlugin")
        endif()
        qt5_import_plugins(qt_libpocketcoinqt INCLUDE Qt5::QCocoaIntegrationPlugin)
        target_compile_definitions(qt_libpocketcoinqt PRIVATE QT_QPA_PLATFORM_COCOA=1)
    else()
        if(NOT TARGET Qt5::QXcbIntegrationPlugin)
            message(FATAL_ERROR "Failed to find Qt static module: QXcbIntegrationPlugin")
        endif()
        qt5_import_plugins(qt_libpocketcoinqt INCLUDE Qt5::QXcbIntegrationPlugin)
        target_compile_definitions(qt_libpocketcoinqt PRIVATE QT_QPA_PLATFORM_XCB=1)
    endif()

    target_compile_definitions(qt_libpocketcoinqt PRIVATE QT_STATICPLUGIN=1)
endif()

add_executable(pocketcoin-qt main.cpp)
#if(APPLE)
#    target_sources(pocketcoin-qt PRIVATE ${POCKETCOIN_MM})
#endif()
if(WIN32)
    target_sources(pocketcoin-qt PRIVATE ${POCKETCOIN_RC})
endif()
target_link_libraries(pocketcoin-qt PRIVATE qt_libpocketcoinqt ${POCKETCOIN_SERVER} ${POCKETCOIN_UTIL} leveldb libpocketcoin_cli OpenSSL::SSL OpenSSL::Crypto)
# target_include_directories(pocketcoin-qt PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
