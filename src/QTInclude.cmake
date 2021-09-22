find_package(Protobuf REQUIRED)
find_package(Qt5 COMPONENTS Widgets LinguistTools Network Core)
# set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QT_TS 
qt/locale/pocketcoin_af.ts
qt/locale/pocketcoin_af_ZA.ts
qt/locale/pocketcoin_ar.ts
qt/locale/pocketcoin_be_BY.ts
qt/locale/pocketcoin_bg_BG.ts
qt/locale/pocketcoin_bg.ts
qt/locale/pocketcoin_ca_ES.ts
qt/locale/pocketcoin_ca.ts
qt/locale/pocketcoin_ca@valencia.ts
qt/locale/pocketcoin_cs.ts
qt/locale/pocketcoin_cy.ts
qt/locale/pocketcoin_da.ts
qt/locale/pocketcoin_de.ts
qt/locale/pocketcoin_el_GR.ts
qt/locale/pocketcoin_el.ts
qt/locale/pocketcoin_en_GB.ts
qt/locale/pocketcoin_en.ts
qt/locale/pocketcoin_eo.ts
qt/locale/pocketcoin_es_AR.ts
qt/locale/pocketcoin_es_CL.ts
qt/locale/pocketcoin_es_CO.ts
qt/locale/pocketcoin_es_DO.ts
qt/locale/pocketcoin_es_ES.ts
qt/locale/pocketcoin_es_MX.ts
qt/locale/pocketcoin_es.ts
qt/locale/pocketcoin_es_UY.ts
qt/locale/pocketcoin_es_VE.ts
qt/locale/pocketcoin_et_EE.ts
qt/locale/pocketcoin_et.ts
qt/locale/pocketcoin_eu_ES.ts
qt/locale/pocketcoin_fa_IR.ts
qt/locale/pocketcoin_fa.ts
qt/locale/pocketcoin_fi.ts
qt/locale/pocketcoin_fr_CA.ts
qt/locale/pocketcoin_fr_FR.ts
qt/locale/pocketcoin_fr.ts
qt/locale/pocketcoin_gl.ts
qt/locale/pocketcoin_he.ts
qt/locale/pocketcoin_hi_IN.ts
qt/locale/pocketcoin_hr.ts
qt/locale/pocketcoin_hu.ts
qt/locale/pocketcoin_id_ID.ts
qt/locale/pocketcoin_it_IT.ts
qt/locale/pocketcoin_it.ts
qt/locale/pocketcoin_ja.ts
qt/locale/pocketcoin_ka.ts
qt/locale/pocketcoin_kk_KZ.ts
qt/locale/pocketcoin_ko_KR.ts
qt/locale/pocketcoin_ku_IQ.ts
qt/locale/pocketcoin_ky.ts
qt/locale/pocketcoin_la.ts
qt/locale/pocketcoin_lt.ts
qt/locale/pocketcoin_lv_LV.ts
qt/locale/pocketcoin_mk_MK.ts
qt/locale/pocketcoin_mn.ts
qt/locale/pocketcoin_ms_MY.ts
qt/locale/pocketcoin_nb.ts
qt/locale/pocketcoin_ne.ts
qt/locale/pocketcoin_nl.ts
qt/locale/pocketcoin_pam.ts
qt/locale/pocketcoin_pl.ts
qt/locale/pocketcoin_pt_BR.ts
qt/locale/pocketcoin_pt_PT.ts
qt/locale/pocketcoin_ro_RO.ts
qt/locale/pocketcoin_ro.ts
qt/locale/pocketcoin_ru_RU.ts
qt/locale/pocketcoin_ru.ts
qt/locale/pocketcoin_sk.ts
qt/locale/pocketcoin_sl_SI.ts
qt/locale/pocketcoin_sq.ts
qt/locale/pocketcoin_sr@latin.ts
qt/locale/pocketcoin_sr.ts
qt/locale/pocketcoin_sv.ts
qt/locale/pocketcoin_ta.ts
qt/locale/pocketcoin_th_TH.ts
qt/locale/pocketcoin_tr_TR.ts
qt/locale/pocketcoin_tr.ts
qt/locale/pocketcoin_uk.ts
qt/locale/pocketcoin_ur_PK.ts
qt/locale/pocketcoin_uz@Cyrl.ts
qt/locale/pocketcoin_vi.ts
qt/locale/pocketcoin_vi_VN.ts
qt/locale/pocketcoin_zh_CN.ts
qt/locale/pocketcoin_zh_HK.ts
qt/locale/pocketcoin_zh.ts
qt/locale/pocketcoin_zh_TW.ts)
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

set(QT_FORMS_UI
qt/forms/addressbookpage.ui
qt/forms/askpassphrasedialog.ui
qt/forms/coincontroldialog.ui
qt/forms/editaddressdialog.ui
qt/forms/updatenotificationdialog.ui
qt/forms/helpmessagedialog.ui
qt/forms/intro.ui
qt/forms/modaloverlay.ui
qt/forms/openuridialog.ui
qt/forms/optionsdialog.ui
qt/forms/overviewpage.ui
qt/forms/receivecoinsdialog.ui
qt/forms/receiverequestdialog.ui
qt/forms/debugwindow.ui
qt/forms/sendcoinsdialog.ui
qt/forms/sendcoinsentry.ui
qt/forms/signverifymessagedialog.ui
qt/forms/transactiondescdialog.ui)
set_source_files_properties(${QT_FORMS_UI} PROPERTIES OUTPUT_LOCATION qt/)


set(QT_MOC_CPP
    qt/moc_addressbookpage.cpp
    qt/moc_addresstablemodel.cpp
    qt/moc_askpassphrasedialog.cpp
    qt/moc_bantablemodel.cpp
    qt/moc_pocketcoinaddressvalidator.cpp
    qt/moc_pocketcoinamountfield.cpp
    qt/moc_pocketcoingui.cpp
    qt/moc_pocketcoinunits.cpp
    qt/moc_callback.cpp
    qt/moc_clientmodel.cpp
    qt/moc_coincontroldialog.cpp
    qt/moc_coincontroltreewidget.cpp
    qt/moc_csvmodelwriter.cpp
    qt/moc_editaddressdialog.cpp
    qt/moc_updatenotificationdialog.cpp
    qt/moc_guiutil.cpp
    qt/moc_intro.cpp
    qt/moc_macnotificationhandler.cpp
    qt/moc_modaloverlay.cpp
    qt/moc_notificator.cpp
    qt/moc_openuridialog.cpp
    qt/moc_optionsdialog.cpp
    qt/moc_optionsmodel.cpp
    qt/moc_overviewpage.cpp
    qt/moc_peertablemodel.cpp
    qt/moc_paymentserver.cpp
    qt/moc_qvalidatedlineedit.cpp
    qt/moc_qvaluecombobox.cpp
    qt/moc_receivecoinsdialog.cpp
    qt/moc_receiverequestdialog.cpp
    qt/moc_recentrequeststablemodel.cpp
    qt/moc_rpcconsole.cpp
    qt/moc_sendcoinsdialog.cpp
    qt/moc_sendcoinsentry.cpp
    qt/moc_signverifymessagedialog.cpp
    qt/moc_splashscreen.cpp
    qt/moc_trafficgraphwidget.cpp
    qt/moc_transactiondesc.cpp
    qt/moc_transactiondescdialog.cpp
    qt/moc_transactionfilterproxy.cpp
    qt/moc_transactiontablemodel.cpp
    qt/moc_transactionview.cpp
    qt/moc_utilitydialog.cpp
    qt/moc_walletframe.cpp
    qt/moc_walletmodel.cpp
    qt/moc_walletview.cpp
        )

set(POCKETCOIN_MM
    qt/macdockiconhandler.mm
    qt/macnotificationhandler.mm)

set(QT_MOC
qt/pocketcoin.cpp
qt/pocketcoinamountfield.cpp
qt/callback.h
qt/intro.cpp
qt/overviewpage.cpp
qt/rpcconsole.cpp)
foreach(QT_MOC_FILE ${QT_MOC})
    string(REPLACE ".cpp" ".moc" QT_MOC_FILE_REAL ${QT_MOC_FILE})
    string(REPLACE ".h" ".moc" QT_MOC_FILE_REAL ${QT_MOC_FILE_REAL})

    list(APPEND QT_MOCS_REAL ${QT_MOC_FILE_REAL})
    qt5_generate_moc(${QT_MOC_FILE} ${QT_MOC_FILE_REAL})
endforeach()
add_custom_target(moc_generated ALL DEPENDS ${QT_MOCS_REAL})

set(QT_QRC_CPP qt/qrc_pocketcoin.cpp)
set(QT_QRC qt/pocketcoin.qrc)
set(QT_QRC_LOCALE_CPP qt/qrc_pocketcoin_locale.cpp)
set(QT_QRC_LOCALE qt/pocketcoin_locale.qrc)
set_source_files_properties(${QT_QRC_LOCALE} PROPERTIES OUTPUT_LOCATION qt/)
# set(PROTOBUF_CC qt/paymentrequest.pb.cc)
# set(PROTOBUF_H qt/paymentrequest.pb.h)
set(PROTOBUF_PROTO qt/paymentrequest.proto)
set_source_files_properties(${PROTOBUF_PROTO} PROPERTIES OUTPUT_LOCATION qt/)
protobuf_generate(OUT_VAR PROTOBUF_H PROTOS ${PROTOBUF_PROTO})
add_custom_target(protobuf_generated ALL DEPENDS ${PROTOBUF_CC} ${PROTOBUF_H})

set(POCKETCOIN_QT_H
qt/addressbookpage.h
qt/addresstablemodel.h
qt/askpassphrasedialog.h
qt/bantablemodel.h
qt/pocketcoinaddressvalidator.h
qt/pocketcoinamountfield.h
qt/pocketcoingui.h
qt/pocketcoinunits.h
qt/callback.h
qt/clientmodel.h
qt/coincontroldialog.h
qt/coincontroltreewidget.h
qt/csvmodelwriter.h
qt/editaddressdialog.h
qt/updatenotificationdialog.h
qt/guiconstants.h
qt/guiutil.h
qt/intro.h
qt/macdockiconhandler.h
qt/macnotificationhandler.h
qt/modaloverlay.h
qt/networkstyle.h
qt/notificator.h
qt/openuridialog.h
qt/optionsdialog.h
qt/optionsmodel.h
qt/overviewpage.h
qt/paymentrequestplus.h
qt/paymentserver.h
qt/peertablemodel.h
qt/platformstyle.h
qt/qvalidatedlineedit.h
qt/qvaluecombobox.h
qt/receivecoinsdialog.h
qt/receiverequestdialog.h
qt/recentrequeststablemodel.h
qt/rpcconsole.h
qt/sendcoinsdialog.h
qt/sendcoinsentry.h
qt/signverifymessagedialog.h
qt/splashscreen.h
qt/trafficgraphwidget.h
qt/transactiondesc.h
qt/transactiondescdialog.h
qt/transactionfilterproxy.h
qt/transactionrecord.h
qt/transactiontablemodel.h
qt/transactionview.h
qt/utilitydialog.h
qt/walletframe.h
qt/walletmodel.h
qt/walletmodeltransaction.h
qt/walletview.h
qt/winshutdownmonitor.h)

# TODO custom target to wait
qt5_wrap_cpp(POCKETCOIN_QT_H_MOC ${POCKETCOIN_QT_H})

set(RES_ICONS
qt/res/icons/add.png
qt/res/icons/address-book.png
qt/res/icons/about.png
qt/res/icons/about_qt.png
qt/res/icons/pocketcoin.ico
qt/res/icons/pocketcoin_testnet.ico
qt/res/icons/pocketcoin.png
qt/res/icons/chevron.png
qt/res/icons/clock1.png
qt/res/icons/clock2.png
qt/res/icons/clock3.png
qt/res/icons/clock4.png
qt/res/icons/clock5.png
qt/res/icons/configure.png
qt/res/icons/connect0.png
qt/res/icons/connect1.png
qt/res/icons/connect2.png
qt/res/icons/connect3.png
qt/res/icons/connect4.png
qt/res/icons/debugwindow.png
qt/res/icons/edit.png
qt/res/icons/editcopy.png
qt/res/icons/editpaste.png
qt/res/icons/export.png
qt/res/icons/eye.png
qt/res/icons/eye_minus.png
qt/res/icons/eye_plus.png
qt/res/icons/filesave.png
qt/res/icons/fontbigger.png
qt/res/icons/fontsmaller.png
qt/res/icons/hd_disabled.png
qt/res/icons/hd_enabled.png
qt/res/icons/history.png
qt/res/icons/info.png
qt/res/icons/key.png
qt/res/icons/lock_closed.png
qt/res/icons/lock_open.png
qt/res/icons/network_disabled.png
qt/res/icons/open.png
qt/res/icons/overview.png
qt/res/icons/proxy.png
qt/res/icons/quit.png
qt/res/icons/receive.png
qt/res/icons/remove.png
qt/res/icons/send.png
qt/res/icons/synced.png
qt/res/icons/transaction0.png
qt/res/icons/transaction2.png
qt/res/icons/transaction_abandoned.png
qt/res/icons/transaction_conflicted.png
qt/res/icons/tx_inout.png
qt/res/icons/tx_input.png
qt/res/icons/tx_output.png
qt/res/icons/tx_mined.png
qt/res/icons/warning.png
qt/res/icons/verify.png)

set(POCKETCOIN_QT_BASE_CPP
qt/bantablemodel.cpp
qt/pocketcoinaddressvalidator.cpp
qt/pocketcoinamountfield.cpp
qt/pocketcoingui.cpp
qt/pocketcoinunits.cpp
qt/clientmodel.cpp
qt/csvmodelwriter.cpp
qt/guiutil.cpp
qt/intro.cpp
qt/modaloverlay.cpp
qt/networkstyle.cpp
qt/notificator.cpp
qt/optionsdialog.cpp
qt/optionsmodel.cpp
qt/peertablemodel.cpp
qt/platformstyle.cpp
qt/qvalidatedlineedit.cpp
qt/qvaluecombobox.cpp
qt/rpcconsole.cpp
qt/splashscreen.cpp
qt/trafficgraphwidget.cpp
qt/utilitydialog.cpp)

set(POCKETCOIN_QT_WINDOWS_CPP winshutdownmonitor.cpp)
set(POCKETCOIN_QT_WALLET_CPP
qt/addressbookpage.cpp
qt/addresstablemodel.cpp
qt/askpassphrasedialog.cpp
qt/coincontroldialog.cpp
qt/coincontroltreewidget.cpp
qt/editaddressdialog.cpp
qt/updatenotificationdialog.cpp
qt/openuridialog.cpp
qt/overviewpage.cpp
qt/paymentrequestplus.cpp
qt/paymentserver.cpp
qt/receivecoinsdialog.cpp
qt/receiverequestdialog.cpp
qt/recentrequeststablemodel.cpp
qt/sendcoinsdialog.cpp
qt/sendcoinsentry.cpp
qt/signverifymessagedialog.cpp
qt/transactiondesc.cpp
qt/transactiondescdialog.cpp
qt/transactionfilterproxy.cpp
qt/transactionrecord.cpp
qt/transactiontablemodel.cpp
qt/transactionview.cpp
qt/walletframe.cpp
qt/walletmodel.cpp
qt/walletmodeltransaction.cpp
qt/walletview.cpp)

set(POCKETCOIN_QT_CPP ${POCKETCOIN_QT_BASE_CPP})
if(WIN32)
    list(APPEND POCKETCOIN_QT_CPP ${POCKETCOIN_QT_WINDOWS_CPP})
endif()
if(NOT DISABLE_WALLET)
    list(APPEND POCKETCOIN_QT_CPP ${POCKETCOIN_QT_WALLET_CPP})
endif()

set(RES_MOVIES qt/res/movies/spinner-000.png
qt/res/movies/spinner-001.png
qt/res/movies/spinner-002.png
qt/res/movies/spinner-003.png
qt/res/movies/spinner-004.png
qt/res/movies/spinner-005.png
qt/res/movies/spinner-006.png
qt/res/movies/spinner-007.png
qt/res/movies/spinner-008.png
qt/res/movies/spinner-009.png
qt/res/movies/spinner-010.png
qt/res/movies/spinner-011.png
qt/res/movies/spinner-012.png
qt/res/movies/spinner-013.png
qt/res/movies/spinner-014.png
qt/res/movies/spinner-015.png
qt/res/movies/spinner-016.png
qt/res/movies/spinner-017.png
qt/res/movies/spinner-018.png
qt/res/movies/spinner-019.png
qt/res/movies/spinner-020.png
qt/res/movies/spinner-021.png
qt/res/movies/spinner-022.png
qt/res/movies/spinner-023.png
qt/res/movies/spinner-024.png
qt/res/movies/spinner-025.png
qt/res/movies/spinner-026.png
qt/res/movies/spinner-027.png
qt/res/movies/spinner-028.png
qt/res/movies/spinner-029.png
qt/res/movies/spinner-030.png
qt/res/movies/spinner-031.png
qt/res/movies/spinner-032.png
qt/res/movies/spinner-033.png
qt/res/movies/spinner-034.png
qt/res/movies/spinner-035.png)

set(POCKETCOIN_RC qt/res/pocketcoin-qt-res.rc)

qt5_create_translation(QT_QM_COMPILED ${QT_TS} OPTIONS)
foreach(QT_QM_COMPILED_RELATIVE_PATH ${QT_QM_COMPILED})

    list(APPEND QT_QM_COMPILED_PATHS ${CMAKE_CURRENT_BINARY_DIR}/${QT_QM_COMPILED_RELATIVE_PATH})
endforeach()

add_custom_target(translations_available ALL DEPENDS ${QT_QM_COMPILED_PATHS})


add_custom_target(translations COMMAND ${CMAKE_COMMAND} -DSRC="${QT_QM_COMPILED_PATHS}" -DDST="${CMAKE_CURRENT_SOURCE_DIR}/${QT_TS_OUTPUT_PATH}" -P ${CMAKE_CURRENT_SOURCE_DIR}/CopyCached.cmake)
add_dependencies(translations translations_available)

# add_custom_target(maketemp COMMAND cp)

message(${CMAKE_CURRENT_BINARY_DIR}/qt)
qt5_add_resources(QT_QRC_COMPILED ${QT_QRC} OPTIONS -name pocketcoin)
qt5_add_resources(QT_QRC_LOCALE_COMPILED qt/pocketcoin_locale.qrc OPTIONS -name pocketcoin_locale)
add_custom_target(resources_available ALL DEPENDS ${QT_QRC_COMPILED})
add_custom_target(resources_locale_available ALL DEPENDS ${QT_QRC_LOCALE_COMPILED})
add_dependencies(resources_locale_available translations)

add_custom_target(resources COMMAND ${CMAKE_COMMAND} -DSRC="${QT_QRC_COMPILED}" -DDST="${CMAKE_CURRENT_BINARY_DIR}/qt/" -P ${CMAKE_CURRENT_SOURCE_DIR}/CopyCached.cmake)
add_dependencies(resources resources_available)
add_custom_target(resources_locale COMMAND ${CMAKE_COMMAND} -DSRC="${QT_QRC_LOCALE_COMPILED}" -DDST="${CMAKE_CURRENT_BINARY_DIR}/qt/" -P ${CMAKE_CURRENT_SOURCE_DIR}/CopyCached.cmake)
add_dependencies(resources_locale resources_locale_available)


qt5_wrap_ui(QT_UI_COMPILED ${QT_FORMS_UI})
add_custom_target(ui_available ALL DEPENDS ${QT_UI_COMPILED})

add_custom_target(ui_done COMMAND ${CMAKE_COMMAND} -DSRC="${QT_UI_COMPILED}" -DDST="${CMAKE_CURRENT_BINARY_DIR}/qt/forms" -P ${CMAKE_CURRENT_SOURCE_DIR}/CopyCached.cmake)
add_dependencies(ui_done ui_available)



add_library(qt_libpocketcoinqt 
                ${POCKETCOIN_QT_CPP}
                ${POCKETCOIN_QT_H}
                # ${QT_FORMS_UI}
                ${QT_QRC_COMPILED}
                ${QT_QRC_LOCALE_COMPILED}
                # ${QT_TS}
                ${PROTOBUF_PROTO}
                ${RES_ICONS}
                ${RES_IMAGES}
                ${RES_MOVIES}
                ${POCKETCOIN_QT_H_MOC}
                ${QT_MOCS_REAL}
                ${PROTOBUF_CC}
                ${PROTOBUF_H}
                # ${QT_QRC_CPP}
                # ${QT_QRC_LOCALE_CPP}
                )
# TODO disable prefix
target_link_libraries(qt_libpocketcoinqt Qt5::Core Qt5::Widgets Qt5::Network OpenSSL::Crypto ${POCKETCOIN_SERVER} leveldb)
add_dependencies(qt_libpocketcoinqt resources resources_locale protobuf_generated ui_done moc_generated)
# TODOO Add checks for this or allow use config as it is in autotools
target_compile_definitions(qt_libpocketcoinqt PRIVATE HAVE_DECL_BSWAP_16=1 HAVE_DECL_BSWAP_32=1 HAVE_DECL_BSWAP_64=1 HAVE_DECL_EVP_MD_CTX_NEW=1)
target_include_directories(qt_libpocketcoinqt PRIVATE ${OPENSSL_INCLUDE_DIR})

# set_property(TARGET qt_libpocketcoinqt PROPERTY AUTORCC ON)


add_executable(pocketcoin-qt qt/pocketcoin.cpp)
if(APPLE)
    target_sources(pocketcoin-qt PRIVATE ${POCKETCOIN_MM} qt/moc_macdockiconhandler.cpp)
endif()
if(WIN32)
    target_sources(pocketcoin-qt PRIVATE ${POCKETCOIN_RC})
endif()
target_link_libraries(pocketcoin-qt PRIVATE qt_libpocketcoinqt ${POCKETCOIN_SERVER} leveldb memenv protobuf::libprotobuf libpocketcoin_cli)
target_include_directories(pocketcoin-qt PRIVATE ${Protobuf_INCLUDE_DIRS})


target_compile_definitions(qt_libpocketcoinqt PRIVATE QT_NO_KEYWORDS)
