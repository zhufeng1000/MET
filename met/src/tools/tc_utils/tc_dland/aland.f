C     ----------------------------------------------
C     ALAND  A collection of routines for computing
C               the distance to land.
C     ----------------------------------------------

      SUBROUTINE ALAND ( DATAFILE, CLON, CLAT, DIST )
C
c  Input: DATAFILE - Location of aland.dat data file.
c         CLON     - Longitude (deg W negative)
c         CLAT     - Latitude (deg N positive)
c  
c  Output: DIST - distance (km) to nearest coastline. DIST is positive if the 
c                 point CLON,CLAT is over water and negative if it is over land.
c
c                 This version is valid for tropical cyclones in the Atlantic,
c                 east, and central North Pacific. 
c
c                 The represenation of the coastline and islands is in the file
c                 aland.dat
c         
C  SUBROUTINE LAND COMPUTES THE DISTANCE (KM) FROM, AND NORMAL BEARING
C  TO, THE NEAREST COASTLINE TO A CYCLONE CENTRAL AT LATITUDE 'CLAT
C  AND LONGITUDE 'CLON'.  COASTLINES ARE STORED IN THE COMMON BLOCK
C  /COAST/ AND FOR UP TO 20 ISLANDS (NUMBER NISL) OF UP TO 30 COASTAL
C  LINE SEGMENTS.  THE INTIIAL POINTS OF EACH SEGMENT ARE STORED
C  IN COUNTERCLOCKWISE ORDER AROUND THE COAST.  THE LAST INDEX IN
C  'XISL' OR 'XCON' IS 1 FOR LONGITUDE AND 2 FOR LATITUDE.
C                                                                
C  THE DISTANCE FROM THE CYCLONE TO THE NEAREST POINT ON EACH COASTAL
C  SEGMENT IS CALCULATED, AND THE MINIMUM ABSOLUTE VALUE IS ACCEPTED
C  AS THE COASTAL DISTANCE FOR THAT PARTICULAR LAND MASS.  NEGATIVE
C  VALUES ARE INLAND.  THE MINIMUM ABSOLUTE VALUE OF ALL COASTAL
C  DISTANCES FOR THE CYCLONE IS RETURNED AS THE COASTAL DISTANCE FOR
C  THIS PARTICULAR CYCLONE.                                      
C                                                                
C       SEVERAL MODIFICATIONS WERE MADE TO THE SUBROUTINE LAND
C       TO FACILITATE FUTURE CHANGES TO THE DATA FILE
C       USED TO DETERMINE A TROPICAL CYCLONE'S DISTANCE FROM LAND. 
C           
C       THE VARIABLES NCONOB AND NISLOB SPECIFY THE LIMITS 
C       FOR THE NUMBER OF POINTS WHICH DEFINE A CONTINENT AND ISLAND 
C       RESPECTIVELY. THE VARIABLE NCOAST IS THE LIMIT FOR THE NUMBER
C       OF ISLANDS. PROVIDED THESE NUMBERS ARE NOT EXCEEDED ANY CHANGES
C       TO THE DATA FILE LAND.DAT SHOULD BE TRANSPARENT TO THE PROGRAM. 
C       5/13/92    J.KAPLAN
C
      PARAMETER (NCOAST=30)
      PARAMETER (NCONOB=400)
      PARAMETER (NISLOB=100)

      CHARACTER DATAFILE* ( 1024 )
C
C      REAL*8 CISL(NCOAST)
      REAL*4 XISL(NCOAST,NISLOB,2), DI(NCOAST), BI(NCOAST),
     1       XCON(NCONOB,2), DWRC(NCONOB), BWRC(NCONOB), 
     2       DWRK(NISLOB), BWRK(NISLOB)
      INTEGER*4 NPT(NCOAST), LT(NCOAST)
      CHARACTER CISL(NCOAST)*8
      LOGICAL*1 FLAG1, LPR
C
      DATA FLAG1/.FALSE./
      DATA LPR  /.FALSE./
      DATA IOPTN /7/
      DATA LUIN  /55/
      save flag1,xisl,xcon,ncon,nisl,npt,lt,cisl
C
      IF( FLAG1 ) GO TO 409
C
C     T H I S   I S   C O A S L I N E   I N P U T **********
C
C     On Linux
      OPEN(LUIN,FILE=DATAFILE,
     +     FORM='FORMATTED',STATUS='OLD')
c
C     ON TYPE42
C      OPEN(LUIN,FILE=DATAFILE,
C     +     FORM='FORMATTED',STATUS='OLD',READONLY)
C
C     READ CONTINENT COASTAL POINTS
C     READ TOTAL NUMBER OF CONTINENT POINTS
C
      READ(LUIN,209,END=995) NCON, CISL(1), LT(1)
C
      IF (NCON.GT.NCONOB) THEN
         WRITE(6,*)'NUMBER OF CONTINENT POINTS EXCEEDS PROGRAM LIMIT
     1              OF ',NCONOB
         STOP
      ENDIF
C
      DO 105 I = 1,NCON
         READ(LUIN,207,END=995) XCON(I,1), XCON(I,2)
  207    FORMAT(F7.1,F6.1)
  105 CONTINUE
C
C     COPY INITIAL POINT INTO NCON+1 ARRAY ADDRESS (WILL BE NEEDED
C     TO COMPUTE DISTANCE AND BEARING FROM LAST POINT IN CLOSED LOOP
C     TO 'NEXT' POINT (WHICH IS THE FIRST POINT ENTERED).
      XCON(NCON+1,1) = XCON(1,1)
      XCON(NCON+1,2) = XCON(1,2)
C
C     OUTPUT THE COASTAL POINTS FOR THE CONTINENT
      IF( LPR ) WRITE(6,305) CISL(1), NCON
  305 FORMAT(/,5X,'COASTLINE OF ',A8,/,5X,'IS DEFINED BY THE FOLLOWING '
     1       ,I3,' POINTS')
C
      IF( LPR )WRITE(6,307) ( I, XCON(I,1), XCON(I,2), I=1,NCON )
  307 FORMAT(6(1X,I3,' (',F6.1,F5.1,')'))
C
C     READ ISLAND NAMES AND COASTAL POINTS
C
      NISL = 0
  200 NISL = NISL + 1
      N= NISL + 1
C       
      READ(LUIN,209,END=407) NPT(N), CISL(N), LT(N)
  209 FORMAT(I3,1X,A8,1X,I1)
C
      NPTS = NPT(N)
C
      DO 109 I = 1,NPTS
         READ(LUIN,207,END=995) XISL(N,I,1), XISL(N,I,2)
  109 CONTINUE
C
C     COPY INITIAL POINT INTO NPTS+1 ARRAY SPACE (WILL BE NEEDED TO 
C     COMPUTE DISTANCE AND BEARING FROM LAST POINT IN CLOSED LOOP TO
C     'NEXT' POINT (WHICH IS THE FIRST POINT ENTERED).              
C                                                                
      XISL(N,NPTS+1,1) = XISL(N,1,1)                             
      XISL(N,NPTS+1,2) = XISL(N,1,2)                             
C                                                                
C     OUTPUT THE NAME AND COASTLINE POINTS FOR THIS ISLAND          
C                                                                
      IF( LPR ) WRITE(6,305) CISL(N), NPT(N)
      NPTS = NPT(N)
      IF( LPR ) WRITE(6,307) (I, XISL(N,I,1), XISL(N,I,2), I=1,NPTS )
      GO TO 200
C
C     COME HERE IF THERE ARE NO MORE ISLANDS TO ENTER...EOF ENCOUNTE
C     ON READ(LUIN,205) AT THE TOP OF THIS INPUT SEGMENT
C
  407 NISL = NISL - 1
      FLAG1 = .TRUE.
      CLOSE(LUIN)
C
C     T H I S   I S   L A N D   N A V I G A T I O N **********
C
  409 CONTINUE
C                                                                
C     PROCESS CONTINENT
C
C     COMPUTE ANGLE OF PREVIOUS SEGMENT IF NEEDED FOR BAY OR PENINSULA
C     COASTLINE DETERMINATION FOR SIGN OF DISTANCE IN LSUB1
      CALL LSUB2(XCON(1,1),XCON(NCON,1),XCON(1,2),XCON(NCON,2),DX0,DY0,
     1           AL0)
C     ANGLE OF PREVIOUS SEGMENT IS DIRECTED WITH ORIGIN AT BEGINNING OF
C     CURRENT SEGMENT
      AL0 = ANGL(AL0+180.)
C
      DO 113 I = 1,NCON
C        COMPUTE DISTANCE FROM BEGINNING POINT TO END POINT OF SEGMENT
C        ( COMPONENTS (DX,DY)  AND FROM BEGINNING POINT TO CYCLONE
C        ( COMPONENTS (DXC,DYC) ).
C
         CALL LSUB2(XCON(I+1,1),XCON(I,1),XCON(I+1,2),XCON(I,2), DX, DY,
     1              AL)
         CALL LSUB2(       CLON,XCON(I,1),       CLAT,XCON(I,2),DXC,DYC,
     1              AC)
C
C        CALL DISTANCE, ANGLE CALCULATION SHELL
C
         CALL LSUB1 ( DX, DY, DXC, DYC, AL0,AL,AC, A, DRA, D )
C
         DWRC(I) = D
         BWRC(I) = A
         AL0 = ANGL(AL+180.)
  113 CONTINUE
C
C     COMPUTE MINIMUM DISTANCE FROM CYCLONE TO CONTINENT
      DM = 1000.
      DO 115 I = 1,NCON
         IF( ABS(DWRC(I)).LT.DM ) IM=I
         DM = ABS(DWRC(IM))
  115 CONTINUE
C
C     ASSIGN CONTINENT DISTANCE TO FIRST ADDRESS IN WORKING ARRAY DI, BI
      DI(1) = DWRC(IM)
      BI(1) = BWRC(IM)
C
C
C     OUTER LOOP FOR NUMBER OF ISLANDS NISL
C
      DO 117 N = 2,NISL+1
         NPTS = NPT(N)                                            
C                                                                
C        LOOP THROUGH NPT(N) COASTAL SEGMENTS OF NTH ISLAND.           
         CALL LSUB2(XISL(N,1,1),XISL(N,NPTS,1),XISL(N,1,2),
     1              XISL(N,NPTS,2),DX0,DY0,AL0)
         AL0 = ANGL(AL0+180.)
C
         DO 119 I = 1,NPTS
C           COMPUTE DISTANCE FROM BEGINNING POINT TO END POINT OF SEGMENT 
C           (DX, DY) AND BEGINNING POINT TO CYCLONE (DXC, DYC)            
C                                                                
            CALL LSUB2(XISL(N,I+1,1),XISL(N,I,1),XISL(N,I+1,2),
     1                 XISL(N,I,2), DX, DY, AL)
            CALL LSUB2(CLON,XISL(N,I,1),CLAT,XISL(N,I,2)
     1                 ,DXC,DYC, AC)
C
            CALL LSUB1 ( DX, DY, DXC, DYC, AL0,AL,AC, A, DRA, D )
            DWRK(I) = D
            BWRK(I) = A
C
            AL0 = ANGL(AL+180.)
  119   CONTINUE
C
        DM = 1000.
        DO 121 I = 1,NPTS
           IF( ABS(DWRK(I)).LT.DM ) IM = I
           DM = ABS(DWRK(IM))
  121   CONTINUE
C
        DI(N) = DWRK(IM)                                         
        BI(N) = BWRK(IM)                                         
  117 CONTINUE
C
      DMIN = 9999.
      DO 116 N=1,NISL+1
         IF( ABS(DI(N)).GE.DMIN ) GO TO 116
         IMN = N
         DMIN= ABS(DI(N))
  116 CONTINUE
C
      DIST = DI(IMN)*111.12
      BRG = BI(IMN)
      RETURN
C
C     ERROR MESSAGES
C
  995 WRITE(6,3995) N, CISL(N)
 3995 FORMAT(///,5X,'END OF FILE WHILE ATTEMPTING TO READ THE COAST POIN
     1TS FOR',/,5X,'ISLAND N=',I2,' WITH NAME ',A8,/,5X,'PROBABLE CAUSE
     2IS MISMATCHED NUMBER OF POINTS SPECIFIED FOR ISLAND COAST AND',/,5
     3X,'ACTUAL NUMBER OF POINTS ON LIST.')
C
      STOP                                                       
      END                                                        
      SUBROUTINE LSUB1 ( DX, DY, DXC, DYC, AL0,AL,AC, A, DRA, D )
      DR = 180./3.141592
C                                                                
C     COMPUTE CYCLONE DISTANCE TO COASTAL SEGMENT
      ACL = ANGL(AC-AL)
      DL = SQRT(DX*DX + DY*DY)
      DC = SQRT(DXC*DXC + DYC*DYC)
      DNC = -DC*SIN(ACL/DR)
      DAC =  DC*COS(ACL/DR)
      DRA =  DAC/DL
C
C     ASSIGN DISTANCE FROM CYCLONE TO COAST
C     IF DISTANCE RATIO IS IN RANGE 0-1, USE NORMAL DISTANCE
C     IF DISTANCE RANGE OUTSIDE ABOVE, USE DISTANCE FROM CYCLONE
C     TO INITIAL POINT OF SEGMENT, WITH SIGN DEPENDING ON THE ANGLE OF
C     THE COAST AT THAT POINT
C
C     DETERMINE DISTANCE AND WHETHER OVER LAND OR WATER
      SGNA = 1.
      IF( ABS(ACL).GE.1.E-5 ) SGNA = ACL/ABS(ACL)
      AC0 = ANGL(AC-AL0)
      ALL0= ANGL(AL0-AL)
C
      DFLAG = 0.
      IF( DNC.LT.0. ) DFLAG = 180.
      AFC = -90. - AC
      AFR = DFLAG- AL
C
      IF( DRA.GT.1. ) GO TO 405
      IF( DRA.LT.0. ) GO TO 407
C
C     DISTANCE IS ALONG NORMAL TO COAST
      D = DNC
      A = AFR
      GO TO 410
C
C     DISTANCE IS APPROXIMATED BY DISTANCE TO CYCLONE, BUT WILL BE
C     DEFINITELY LONGER THAN, AND THEREFORE SUPERSEDED BY, THE
C     DISTANCE FROM THE INITIAL POINT OF THE NEXT COASTAL SEGMENT
  405 D = -DC*SGNA
      A = AFC
      GO TO 410
C
C     DISTANCE IS THAT FROM INITIAL POINT TO CYCLONE.  POINT IS OVER
C     WATER IF BEARING OF VECTOR TO CYCLONE IS LESS THAN BEARING
C     OF PREVIOUS COASTAL SEGMENT, OR IF TO RIGHT OF CURRENT COASTAL
C     SEGMENT
  407 D = -DC*SGNA
      A = AFC
C
C     DISTANCE OFFSHORE/ONSHORE IS NOW SET UP RELATIVE TO CURRENT COASTAL
C     SEGMENT...NEGATIVE IS ONSHORE, OR TO THE LEFT.  CORRECT DEPENDING
C     UPON ORIENTATION OF PREVIOUS COASTAL SEGMENT
C
      IF( D.LE.0. ) GO TO 409
C     CYCLONE IS OFFSHORE (TO RIGHT)
      IF( AC0.LE.0.AND.ALL0.LE.0. ) D = -D
      GO TO 410
C     CYCLONE IS ONSHORE (TO LEFT)
  409 IF( AC0.GT.0.AND.ALL0.GT.0 )  D = -D
C
C     TRANSFORM BEARING FROM THE -180 TO +180 RANGE USED INTERNALLY
C     TO THE 0 TO 360 RANGE FOR DISPLAY PURPOSES.
  410 IF( A.LT.0. ) A = A + 360.
C
      RETURN
      END
      SUBROUTINE LSUB2( X2, X1, Y2, Y1, DX, DY, A )
      DR = 180./3.141592
      DY =  Y2-Y1
      DX = (X2-X1)*COS((Y2+Y1)/(2.*DR))
      A  = 0.
      DD = ABS(DX)+ABS(DY)
      IF( DD.LE.1.E-5 ) RETURN
      A  = ATAN2(DY,DX)*DR
      RETURN
      END
C
      REAL FUNCTION ANGL(A)
      ANGL = A
      IF( A.LE.-180.) ANGL = A + 360.
      IF( A.GT. 180.) ANGL = A - 360.
      RETURN
      END
C
C
C
