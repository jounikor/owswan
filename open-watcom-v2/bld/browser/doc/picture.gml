:INCLUDE file='FMTMACRO'.
:INCLUDE file='GMLMACS'.
:INCLUDE file='SYMBOLS'.
:GDOC.
.* ���������������������������������������������������������������Ŀ
.* � Copyright by WATCOM Systems Inc. 1990.  All rights reserved.  �
.* � No part of this publication may be reproduced in any form or  �
.* � by any means - graphic, electronic or mechanical, including   �
.* � photocopying, recording, taping or information storage and    �
.* � retrieval systems. 					   �
.* �����������������������������������������������������������������
.*
:BODY.
.np
This is a paragraph.
See the picture in
:figref refid='file'..
:P.
:graphic depth='3.2i' xoff='0.5i' yoff='4.4i' scale=70 file='test.eps'
:fig id='file' frame=none depth=0 place=inline.
:figcap.This is a picture of a thing.
:efig.
.np
:eGDOC.
