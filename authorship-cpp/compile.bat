@echo off
g++ -std=c++17 ^
    -static-libgcc -static-libstdc++ ^
    -O2 ^
    src/main.cpp ^
    src/tokenizer.cpp ^
    src/features.cpp ^
    src/knn_model.cpp ^
    src/similarity.cpp ^
    src/gemini.cpp ^
    src/google_classroom.cpp ^
    src/audit_log.cpp ^
    src/gui.cpp ^
    src/gui_sync_sheet.cpp ^
    src/gui_assign_dialog.cpp ^
    src/gui_help_carousel.cpp ^
    src/gui_welcome.cpp ^
    src/gui_overview.cpp ^
    src/gui_flagged.cpp ^
    src/gui_students.cpp ^
    -I include/ ^
    -lwinhttp -lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lws2_32 -lmsimg32 -lgdiplus ^
    -o authorship.exe