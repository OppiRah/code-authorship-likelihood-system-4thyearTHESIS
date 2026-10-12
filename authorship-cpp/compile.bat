@echo off
REM Resources first: icon, manifest, version info (res\calss.rc,
REM numbers from include\version.h). Linked in as calss_res.o.
windres -I include -O coff res/calss.rc -o calss_res.o || exit /b 1
g++ -std=c++17 ^
    -static -static-libgcc -static-libstdc++ ^
    -O2 -mwindows ^
    src/main.cpp ^
    src/analysis_pipeline.cpp ^
    src/student_report.cpp ^
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
    src/gui_source_view.cpp ^
    src/gui_ai_settings.cpp ^
    src/secret_store.cpp ^
    src/app_paths.cpp ^
    src/workspace_state.cpp ^
    src/gui_workspace.cpp ^
    calss_res.o ^
    -I include/ ^
    -lwinhttp -lgdi32 -lcomdlg32 -lole32 -lshell32 -lcomctl32 -lws2_32 -lmsimg32 -lgdiplus -ladvapi32 ^
    -o authorship.exe
