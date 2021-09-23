set(QT_FORMS_UI
        addressbookpage.ui
        askpassphrasedialog.ui
        coincontroldialog.ui
        editaddressdialog.ui
        updatenotificationdialog.ui
        helpmessagedialog.ui
        intro.ui
        modaloverlay.ui
        openuridialog.ui
        optionsdialog.ui
        overviewpage.ui
        receivecoinsdialog.ui
        receiverequestdialog.ui
        debugwindow.ui
        sendcoinsdialog.ui
        sendcoinsentry.ui
        signverifymessagedialog.ui
        transactiondescdialog.ui)

qt5_wrap_ui(QT_UI_COMPILED ${QT_FORMS_UI})
add_custom_target(ui_done ALL DEPENDS ${QT_UI_COMPILED})