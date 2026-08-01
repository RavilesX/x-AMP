option(USE_QMMP_DIALOG "Qmmp file dialog" ON)
option(USE_TWO_PANEL_DIALOG "Two-panel file dialog" ON)

if(USE_QMMP_DIALOG)
    add_subdirectory(QmmpFileDialog)
endif(USE_QMMP_DIALOG)

if(USE_TWO_PANEL_DIALOG)
    add_subdirectory(TwoPanelFileDialog)
endif(USE_TWO_PANEL_DIALOG)
