# DESFire Authentication Comprehensive Flowcharts

This document provides comprehensive flowcharts that visually represent the DESFire authentication process, system architecture, and library interactions.

## Complete DESFire Authentication System

```mermaid
flowchart TB
    start[Start Authentication Process] --> detect[Detect Card Presence]

    subgraph "Hardware Layer"
        detect --> rfField[Activate RF Field]
        rfField --> antiCollision[ISO14443-A Anti-collision]
        antiCollision --> readCardInfo[Read Card Type & Info]
    end

    readCardInfo --> cardType{Card Type?}

    cardType -->|Classic| uidRead[Read 4-byte UID]
    uidRead --> classicAuth[Basic Authentication]
    classicAuth --> uidLookup[Compare UID to Database]

    cardType -->|DESFire| desOp{Operation Mode?}

    desOp -->|Default Mode| readDUID[Read 7-byte UID]
    readDUID --> selectApp[Select Application]

    subgraph "Application-level Authentication"
        selectApp --> getDivKey[Derive Application Key]
        getDivKey --> appAuth[Authentication Process]

        subgraph "Authentication Sequence"
            appAuth --> authCmd[Send Authenticate Command]
            authCmd --> decryptRndB[Receive & Decrypt RndB]
            decryptRndB --> genRndA[Generate RndA]
            genRndA --> rotateRndB[Rotate RndB]
            rotateRndB --> sendCrypto[Send Encrypted RndA+Rotated RndB]
            sendCrypto --> verifyRndA[Verify Rotated RndA Response]
        end

        appAuth --> genSessionKey[Generate Session Keys]
        genSessionKey --> readSecretFile[Read Secret File with Session Key]
        readSecretFile --> verifySecret[Verify Secret Value]
    end

    desOp -->|Random ID Mode| readRandom[Read Random ID]

    subgraph "PICC-level Authentication"
        readRandom --> piccAuth[Authenticate with PICC Master Key]

        subgraph "PICC Auth Sequence"
            piccAuth --> piccAuthCmd[Send Authenticate Command]
            piccAuthCmd --> piccDecryptRndB[Receive & Decrypt RndB]
            piccDecryptRndB --> piccGenRndA[Generate RndA]
            piccGenRndA --> piccRotateRndB[Rotate RndB]
            piccRotateRndB --> piccSendCrypto[Send Encrypted RndA+Rotated RndB]
            piccSendCrypto --> piccVerifyRndA[Verify Rotated RndA Response]
        end

        piccAuth --> piccSessionKey[Generate Session Keys]
        piccSessionKey --> getRealUID[Get Real UID]
        getRealUID --> uidRealLookup[Compare Real UID to Database]
    end

    uidLookup --> decision{Authorized?}
    verifySecret --> decision
    uidRealLookup --> decision

    decision -->|Yes| accessGranted[Access Granted]
    decision -->|No| accessDenied[Access Denied]

    accessGranted --> actionPerformed[Perform Requested Action]
    accessDenied --> failureLogged[Log Access Failure]

    actionPerformed --> terminateSession[Terminate Secure Session]
    failureLogged --> terminateSession

    terminateSession --> deactivateRF[Deactivate RF Field]
    deactivateRF --> end[End Process]
```

## Library Architecture and Data Flow

```mermaid
flowchart TB
    application[User Application] --> libraryAPI[DESFire Library API]

    subgraph "DESFire Library"
        libraryAPI --> coreFunctions[Core Functions]

        subgraph "Function Categories"
            coreFunctions --> cardInfo[Card Information]
            coreFunctions --> appMgmt[Application Management]
            coreFunctions --> fileMgmt[File Management]
            coreFunctions --> keyMgmt[Key Management]
            coreFunctions --> authMgmt[Authentication]
        end

        authMgmt --> authProcess[Authentication Process]
        authProcess --> sessionKeyGen[Session Key Generation]

        sessionKeyGen --> secureChannel[Secure Channel]
        secureChannel --> encryptionLayer[Encryption Layer]
        secureChannel --> cmacLayer[CMAC Generation & Verification]

        encryptionLayer --> cipherModes[Cipher Modes]
        cipherModes --> desCipher[DES/3K3DES Implementation]
        cipherModes --> aesCipher[AES Implementation]

        cmacLayer --> macAlgorithms[MAC Algorithms]
        macAlgorithms --> desCmac[DES-MAC]
        macAlgorithms --> aesCmac[CMAC]
    end

    libraryAPI --> pn532API[PN532 Library API]

    subgraph "PN532 Library"
        pn532API --> commProtocol[Communication Protocol]

        commProtocol --> cmdFrames[Command Frames]
        commProtocol --> respFrames[Response Frames]
        commProtocol --> errorHandling[Error Handling]

        cmdFrames --> cmdBuild[Frame Building]
        respFrames --> respParse[Frame Parsing]
        errorHandling --> recover[Recovery Mechanisms]

        cmdBuild --> lowLevel[Low-Level Communication]
        respParse --> lowLevel
        recover --> lowLevel
    end

    pn532API --> hwInterface[Hardware Interface]

    subgraph "Hardware Communication"
        hwInterface --> spiMode[SPI Mode]
        hwInterface --> i2cMode[I2C Mode]

        spiMode --> softSPI[Software SPI (10kHz)]
        spiMode --> hardSPI[Hardware SPI]

        softSPI --> signals[Signal Management]
        hardSPI --> signals
        i2cMode --> signals
    end

    signals --> nfcChip[PN532 NFC Chip]
    nfcChip --> rfInterface[RF Interface]
    rfInterface --> card[DESFire Card]
```

## Card Personalization Process

```mermaid
flowchart TB
    start[Begin Card Personalization] --> detectCard[Detect New Card]
    detectCard --> readInfo[Read Card Information]

    readInfo --> cardType{Card Type?}

    cardType -->|Classic| classicFlow[Classic Mode Personalization]
    classicFlow --> readClassicUID[Read 4-byte UID]
    readClassicUID --> storeClassicUID[Store UID in Database]

    cardType -->|DESFire| desFlow[DESFire Mode Personalization]
    desFlow --> opMode{Operation Mode?}

    opMode -->|Default Mode| defaultMode[Default Mode Setup]
    defaultMode --> readDesUID[Read 7-byte UID]
    readDesUID --> createDivData[Create Diversification Data]
    createDivData --> deriveMasterKey[Derive PICC Master Key]
    deriveMasterKey --> deriveAppKey[Derive Application Key]
    deriveAppKey --> deriveFileKey[Derive File Key]

    deriveFileKey --> defaultAuth[Authenticate with Default PICC Key]
    defaultAuth --> changePICC[Change PICC Master Key]
    changePICC --> createApp[Create Application]
    createApp --> setAppKeys[Set Application Keys]
    setAppKeys --> createFile[Create Data File]
    createFile --> writeSecret[Write Secret Value]
    writeSecret --> storeInfoDB[Store UID & Keys in Database]

    opMode -->|Random Mode| randomMode[Random Mode Setup]
    randomMode --> defaultRndAuth[Authenticate with Default PICC Key]
    defaultRndAuth --> changePICCRnd[Change PICC Master Key]
    changePICCRnd --> enableRandom[Enable Random ID Forever]
    enableRandom --> getRealUID[Get Real UID]
    getRealUID --> storeRealUID[Store Real UID in Database]

    storeClassicUID --> end[End Personalization]
    storeInfoDB --> end
    storeRealUID --> end
```

## Card Access Process - All Modes

```mermaid
flowchart TB
    start[Start Access Process] --> detectCard[Detect Card]
    detectCard --> cardInfo[Get Card Information]

    cardInfo --> cardType{Card Type?}

    cardType -->|Unsupported| rejected[Access Rejected - Unsupported Card]
    cardType -->|Classic| classicProcess[Process Classic Card]
    cardType -->|DESFire| desProcess[Process DESFire Card]

    classicProcess --> readUID[Read 4-byte UID]
    readUID --> lookupUID[Look Up UID in Database]
    lookupUID --> uidFound{UID Authorized?}

    uidFound -->|Yes| classicAccess[Access Granted]
    uidFound -->|No| classicReject[Access Rejected]

    desProcess --> opMode{Operation Mode?}

    opMode -->|Default Mode| defaultProcess[Default Mode Process]
    defaultProcess --> readDUID[Read 7-byte UID]
    readDUID --> selectApp[Select Application]
    selectApp --> authErr{Authentication Error?}

    authErr -->|Yes| authRetry{Retry Count < 3?}
    authRetry -->|Yes| selectApp
    authRetry -->|No| authFail[Authentication Failed]

    authErr -->|No| readSecretFile[Read Secret Value]
    readSecretFile --> verifySecret{Secret Valid?}

    verifySecret -->|Yes| defaultAccess[Access Granted]
    verifySecret -->|No| defaultReject[Access Rejected]

    opMode -->|Random Mode| randomProcess[Random Mode Process]
    randomProcess --> randomAuth[Authenticate with PICC Key]
    randomAuth --> randomErr{Authentication Error?}

    randomErr -->|Yes| randomRetry{Retry Count < 3?}
    randomRetry -->|Yes| randomAuth
    randomRetry -->|No| randomFail[Authentication Failed]

    randomErr -->|No| getUID[Get Real UID]
    getUID --> verifyUID{UID Authorized?}

    verifyUID -->|Yes| randomAccess[Access Granted]
    verifyUID -->|No| randomReject[Access Rejected]

    classicAccess --> accessGranted[Grant Access]
    defaultAccess --> accessGranted
    randomAccess --> accessGranted

    classicReject --> accessDenied[Deny Access]
    defaultReject --> accessDenied
    randomReject --> accessDenied
    authFail --> accessDenied
    randomFail --> accessDenied
    rejected --> accessDenied

    accessGranted --> logAccess[Log Successful Access]
    accessDenied --> logDenied[Log Failed Access]

    logAccess --> end[End Process]
    logDenied --> end
```

## Cryptographic Operations Flowchart

```mermaid
flowchart TB
    start[Start Crypto Operations] --> keyType{Key Type?}

    keyType -->|DES| desOps[DES Operations]
    keyType -->|2K3DES| des2kOps[2K3DES Operations]
    keyType -->|3K3DES| des3kOps[3K3DES Operations]
    keyType -->|AES| aesOps[AES Operations]

    desOps --> blockSize1[8-byte Block Size]
    des2kOps --> blockSize1
    des3kOps --> blockSize1
    aesOps --> blockSize2[16-byte Block Size]

    blockSize1 --> cbcMode1[CBC Mode]
    blockSize2 --> cbcMode2[CBC Mode]

    cbcMode1 --> ivInitDes[IV = All Zeros]
    cbcMode2 --> ivInitAes[IV = All Zeros]

    ivInitDes --> authDes[Authentication]
    ivInitAes --> authAes[Authentication]

    authDes --> randDes[Generate/Encrypt Random Numbers]
    authAes --> randAes[Generate/Encrypt Random Numbers]

    randDes --> desSessKey[Generate DES Session Keys]
    randAes --> aesSessKey[Generate AES Session Keys]

    desSessKey --> secComm1[Secure Communication]
    aesSessKey --> secComm2[Secure Communication]

    secComm1 --> updateIV1[Update IV After Each Command/Response]
    secComm2 --> updateIV2[Update IV After Each Command/Response]

    updateIV1 --> cmacDes[Calculate DES-MAC]
    updateIV2 --> cmacAes[Calculate CMAC]

    cmacDes --> verifyCmac1[Verify Command/Response Integrity]
    cmacAes --> verifyCmac2[Verify Command/Response Integrity]

    verifyCmac1 --> end[End Crypto Operations]
    verifyCmac2 --> end
```

## Error Handling and Recovery Flowchart

```mermaid
flowchart TB
    start[Normal Operation] --> error{Error Detected?}

    error -->|No| continue[Continue Operation]
    error -->|Yes| errorType{Error Type?}

    errorType -->|Communication Error| commError[Communication Error Handling]
    errorType -->|Timeout Error| timeoutError[Timeout Error Handling]
    errorType -->|Integrity Error| integrityError[Integrity Error Handling]
    errorType -->|Authentication Error| authError[Authentication Error Handling]
    errorType -->|Operation Error| opError[Operation Error Handling]

    commError --> resetPN532[Reset PN532 Chip]
    resetPN532 --> reinitComm[Reinitialize Communication]
    reinitComm --> retryComm{Retry Count < 3?}

    retryComm -->|Yes| retryCommOp[Retry Operation]
    retryComm -->|No| failComm[Report Communication Failure]

    timeoutError --> rfOff[Turn Off RF Field]
    rfOff --> shortDelay[Short Delay]
    shortDelay --> rfOn[Turn On RF Field]
    rfOn --> redetectCard[Re-detect Card]
    redetectCard --> retryTimeout{Retry Count < 3?}

    retryTimeout -->|Yes| retryTimeoutOp[Retry Operation]
    retryTimeout -->|No| failTimeout[Report Timeout Failure]

    integrityError --> resetSession[Reset Secure Session]
    resetSession --> reinitAuth[Reinitialize Authentication]
    reinitAuth --> retryIntegrity{Retry Count < 3?}

    retryIntegrity -->|Yes| retryIntegrityOp[Retry Operation]
    retryIntegrity -->|No| failIntegrity[Report Integrity Failure]

    authError --> resetAuthState[Reset Authentication State]
    resetAuthState --> retryAuth{Retry Count < 3?}

    retryAuth -->|Yes| retryAuthOp[Retry Authentication]
    retryAuth -->|No| failAuth[Report Authentication Failure]

    opError --> analyzeError[Analyze Operation Error]
    analyzeError --> errorRecoverable{Error Recoverable?}

    errorRecoverable -->|Yes| retryOp{Retry Count < 3?}
    errorRecoverable -->|No| failOp[Report Operation Failure]

    retryOp -->|Yes| retryOperation[Retry Operation]
    retryOp -->|No| failRetry[Report Retry Limit Reached]

    retryCommOp --> continue
    retryTimeoutOp --> continue
    retryIntegrityOp --> continue
    retryAuthOp --> continue
    retryOperation --> continue

    failComm --> report[Report Error to User]
    failTimeout --> report
    failIntegrity --> report
    failAuth --> report
    failOp --> report
    failRetry --> report

    report --> end[End Operation]
    continue --> end
```

These flowcharts provide a comprehensive visualization of the different aspects of DESFire authentication, from hardware communication to cryptographic operations, card personalization, and error handling procedures.
