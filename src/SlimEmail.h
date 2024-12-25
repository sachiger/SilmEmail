/*
 * header file for slim and simple email sending
 *
 * in this file:    GenericEmailSend; sendSlimEmail; ServerResponseAnlysis; sendCommand; base64Encode; 
 *                  ParseReplaceBR; PrintEmailMessage; getVersion; EmailBegin;
 * 
 * V0 25-XII-2024   []
 */
#ifndef SlimEmail_h
    #define SlimEmail_h
    
    #include    "EmailConfiguration.h"

    #ifndef _DEBUGON                                // enable debug prints
        #define _DEBUGON 0
    #endif  //DEBUGON
    #ifndef _LOGGME                                 // enable log prints
        #define _LOGGME 1
    #endif  //_LOGGME
    
    #define Max_Retries 3                           // number of retries to connect to server
    #define EmailPayloadBufferSize  100             // size of payload (addresses, subject) payload bufffer
    #define EmailServerConnectionTimeout    5000    // delay in mS before stopping to wait for the rerver's response
    #define ServerResponseBufLen  512               // size of server's response message buffer
    #define ErrorBufferLen 50                       // length of email error reason buffer

    struct EmailControl  {
        bool    ConnectionSessionOn;                // flag to indicate email session ongoing
        bool    FlagEmailEvent;                     // flag to set EmailSend time event
        bool    EmailError;                         // flag to indicate error on email send
        uint8_t retries;                            // connection retries counter
        uint8_t EmailType;                          // email type
        uint8_t Payload;                            // Variable for application use (like error number)
        uint8_t RC;                                 // return code. see <GmailErrors>
        unsigned long EmailMillis;                  // time counter for email send duration
        unsigned long TimeSinceStarted;             // keeps the reset timestamp
        char*   Subject;                            // pointer to email's subject buffer
        char*   Body;                               // pointer to email's body buffer
        char*   SpecialMessageBuffer;               // special messages buffer
        char*   ErrorBuffer;                        // buffer to hold Gmail error message
    };
    
    #include    "Clock.h"

    enum  GmailErrors {
        Could_not_connect=0,                    // could not connect
        email_sent=1,                           // message sent successfully
        Authentication_Error=2,                 // Error creating address list
        Email_Content_Error=3,                  // Error creating message
        Send_Error=4,                           // Error sending message
        TooManyRetries_Error=5,                 // Failed to connect too many times
        Program_Error=6                         // unknown program error
    };
    //****************************************************************************************/
    class SlimEmail {
        public:
            SlimEmail(uint8_t CeaseEmail, uint8_t PrintSession, uint8_t PrintConnection);       		// constructor
            EmailControl GenericEmailSend(EmailControl _EmailContIn, TimePack _SysEClock);
            uint8_t     sendSlimEmail(const char* recipient, char* Msg_Subject, char* Msg_Body, char* responseBuffer, bool PrintEcho);
            uint8_t     ServerResponseAnlysis(char* buffer);
            char*       sendCommand(char* cmd, bool Wait4Response,char* buffer, uint16_t bufferSize, bool PrintEcho);
            void        base64Encode(const char* input, char* output, size_t outputSize);
            char*       ParseReplaceBR(char* buff) ;
            void        PrintEmailMessage(EmailControl _EmailContIn,TimePack _SysEClock,bool printMe);
            const char* getVersion();
            EmailControl EmailBegin(EmailControl EmailControl, char* buffer, uint8_t DefaultEmailType);
            #if  BACKUPVERSION==1
                uint8_t sendSlimEmailOriginal(const char* recipient, char* subject, char* body,TimePack _SysEClock, bool PrintEcho);
                void sendCommandOriginal(char* cmd, bool Wait4Response, bool PrintEcho);
            #endif  //BACKUPVERSION==1
        private:
            uint8_t _CeaseEmailSend;                    // simulate send without sending
            uint8_t _PrintEmailSession;                 // print the timing of connection to server stages 
            uint8_t _PrintConnectionTime;               // print dialouge with the server
    };

#endif  //SlimEmail_h