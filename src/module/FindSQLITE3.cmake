if (SQLITE3_Enabled)
    message("SQLITE3_FOUND is found..." )
    find(SQLITE3 sqlite3.h sqlite3)
endif()