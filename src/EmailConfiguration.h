/*
 * Configuration header file for Gamil access
 *
 * V0 25-XII-2024   []
 */
#ifndef EmailConfiguration_h
    #define EmailConfiguration_h 1
    
    #ifndef SimpleEmailVersion
        #define SimpleEmailVersion  "0.0.x"     // library version
    #endif  //SimpleEmailVersion
    #define BACKUPVERSION   0                   // {1 for backup,0 operational}
    //#include    "Main.h"

    // The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com 
    #define SMTP_HOST "smtp.gmail.com"
    #define SMTP_PORT 465 
    #define UserDomain  "192.168.7.1"

    // The log in and sender credentials 
    #define AUTHOR_EMAIL "simha.gerlitz@gmail.com"
    #define AUTHOR_PASSWORD "xsqkcqfbvvlzifjn"
    #define AUTHOR_NAME  "IoT Server"

    // Recipient email address
    #define RECIPIENT_EMAIL "sachi@gerlitz.co.il"
    #define RECIPIENT_NAME  "Sachi Gerlitz"
    #define RECIPIENT_EMAIL1 "sachi@gerlitz.co.il"
    #define RECIPIENT_NAME1 "Sachi Gerlitz"
    #define CC_EMAIL        "sachi.gerlitz@gmail.com"
    #define CC_NAME         "My name"
#endif  //EmailConfiguration_h