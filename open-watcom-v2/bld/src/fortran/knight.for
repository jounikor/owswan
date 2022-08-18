*      The Knight's Tour
*        by Jack Schueler

*$noextensions
*$nocheck
      CHARACTER REPLY*20, LINE(2)*41
      INTEGER I, J, D, SP, OFFSET, TI, TJ, BOARD
      INTEGER ISTACK, JSTACK, DSTACK, DI, DJ
      DIMENSION ISTACK(0:64), JSTACK(0:64), DSTACK(0:64)
      DIMENSION DI(0:11), DJ(0:11), BOARD(0:11, 0:11)
      DATA DI/2, 1, -1, -2, -2, -1, 1, 2, 2, 1, -1, -2/
      DATA DJ/-1, -2, -2, -1, 1, 2, 2, 1, -1, -2, -2, -1/
      PRINT 100
100   FORMAT(
     1'���������������������������������������Ŀ',/,
     2'� K  � N  � I  � G  � H  � T  � ''  � S  �',/,
     3'���������������������������������������Ĵ',/,
     4'�    �    �    �   �    �    �    �    �',/,
     5'���������������������������������������Ĵ',/,
     6'�    �    � T  � O  � U  � R  �    �    �',/,
     7'���������������������������������������������������������ͻ',/,
     8'��This program attempts to find a tour of the chess board �',/,
     9'öusing the knight''s move to cover all squares once and   �',/,
     A'��only once.  The algorithm uses a heuristic.  Given a    �',/,
     B'ösufficient amount of time (sometimes many years) it will�',/,
     C'��find a solution.  Fortunately solutions to many starting�',/,
     D'öpositions are found fairly quickly (e.g., 4,3).         �',/,
     E'���������������������������������������������������������ͼ',/,
     F'���������������������������������������Ĵ',/,
     G'�    � W  � A  � T  � C  � O  � M  �    �',/,
     H'�����������������������������������������' )

*--== What do tours such as those starting at (5,5), (6,6) and (7,7)
*          prove about the knight's tour? ==--

      LOOP
*--== Initialize edge positions to "invalid" ==--
          DO I = 0, 11
              DO J = 0, 11
                  BOARD( I, J ) = -1
              END DO
          END DO
*--== Initialize board positions to "not taken" ==--
          DO I = 2, 9
              DO J = 2, 9
                  BOARD( I, J ) = 0
              END DO
          END DO
*--== Get starting position ==--
          PRINT *,' '
          PRINT *,'Enter the starting row,column for the tour to begin.'
          PRINT *,'(1,1) is the top left corner.  Enter two integers'
          PRINT *,'separated by a comma or type ''QUIT''.'
          WRITE( *, '(A,$)' ) 'Row,Column: '
          READ( *, '(A)', END=99, ERR=99 ) REPLY
          READ( REPLY, *, END=99, ERR=99 ) I, J
          IF( I .GT. 8 .OR.  J .GT. 8 ) GO TO 99
*--== Find a solution ==--
          I = I+1
          J = J+1
          D = -1
          SP = 1
          LOOP :L1
              IF (I .LE. J) THEN
                  OFFSET = 4
                ELSE
                  OFFSET = 0
              END IF
              GUESS :G1
                  LOOP :L2
                      D = D + 1
                      IF( D .EQ. 8 ) QUIT :G1
                      TI = I + DI( D + OFFSET )
                      TJ = J + DJ( D + OFFSET )
                  UNTIL( BOARD( TI, TJ ) .EQ. 0 )
                  ISTACK( SP ) = I
                  JSTACK( SP ) = J
                  DSTACK( SP ) = D
                  BOARD( I, J ) = SP
                  I = TI
                  J = TJ
                  SP = SP + 1
                  D = -1
              ADMIT
                  SP = SP - 1
                  BOARD( I, J ) = 0
                  I = ISTACK( SP )
                  J = JSTACK( SP )
                  D = DSTACK( SP )
              END GUESS
          UNTIL( SP .EQ. 64 )
          BOARD( I, J ) = 64
          J = J - 1
*--== Print final board ==--
          PRINT *,' '
          PRINT *,'Solution of Knight''s Tour by WATFOR-77'
          PRINT *,'          1=Start    ()=Finish'
          PRINT *,'���������������������������������������Ŀ'
          DO K = 2, 8
              WRITE(LINE, 101) (BOARD( K, L ),L=2,9)
              IF( I .EQ. K )LINE(1)(J*5-2:J*5)='()'
              WRITE(*, '(A41)') LINE
          ENDDO
          WRITE(LINE, 102)(BOARD( 9, L ),L=2,9)
          IF( I .EQ. 9 )LINE(1)(J*5-2:J*5)='()'
          WRITE(*, '(A41)') LINE
      ENDLOOP
101       FORMAT(   '� ',I2,' � ',I2,' � ',I2,' � ',I2,
     1    ' � ',I2,' � ',I2,' � ',I2,' � ',I2,' �',/,
     2    '���������������������������������������Ĵ')
102       FORMAT(   '� ',I2,' � ',I2,' � ',I2,' � ',I2,
     1    ' � ',I2,' � ',I2,' � ',I2,' � ',I2,' �',/,
     2    '�����������������������������������������')
99    END
