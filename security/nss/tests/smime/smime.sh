#! /bin/sh  
#
# This is just a quick script so we can still run our testcases.
# Longer term we need a scriptable test environment..
#
. ../common/init.sh
CURDIR=`pwd`

SMIMEDIR=${HOSTDIR}/smime
CADIR=${SMIMEDIR}/cadir
ALICEDIR=${SMIMEDIR}/alicedir
BOBDIR=${SMIMEDIR}/bobdir

echo "<HTML><BODY>" >> ${RESULTS}

SONMI_DEBUG=ON	#we see starnge problems on hpux 64 - save all output
		# for now

#temporary files
if [ -n "$SONMI_DEBUG" -a "$SONMI_DEBUG" = "ON" ]
then
	TMP=${SMIMEDIR}
	PWFILE=${TMP}/tests.pw
	CERTSCRIPT=${TMP}/tests_certs
	NOISE_FILE=${TMP}/tests_noise
	CERTUTILOUT=${TMP}/certutil_out

	TEMPFILES=""
else
	TMP=${TMP-/tmp}
	PWFILE=${TMP}/tests.pw.$$
	CERTSCRIPT=${TMP}/tests_certs.$$
	NOISE_FILE=${TMP}/tests_noise.$$
	CERTUTILOUT=${TMP}/certutil_out.$$

	TEMPFILES="${PWFILE} ${CERTSCRIPT} ${NOISE_FILE} ${CERTUTILOUT}"
	#
	# should also try to kill any running server
	#
	trap "rm -f ${TEMPFILES};  exit"  2 3
fi

mkdir -p ${SMIMEDIR}
mkdir -p ${CADIR}
mkdir -p ${ALICEDIR}
mkdir -p ${BOBDIR}
cd ${CADIR}

# Generate noise for our CA cert.
#
# NOTE: these keys are only suitable for testing, as this whole thing bypasses
# the entropy gathering. Don't use this method to generate keys and certs for
# product use or deployment.
#
ps -efl > ${NOISE_FILE} 2>&1
ps aux >> ${NOISE_FILE} 2>&1
netstat >> ${NOISE_FILE} 2>&1
date >> ${NOISE_FILE} 2>&1

#
# build the TEMP CA used for testing purposes
# 
echo "<TABLE BORDER=1><TR><TH COLSPAN=3>Certutil Tests</TH></TR>" >> ${RESULTS}
echo "<TR><TH width=500>Test Case</TH><TH width=50>Result</TH></TR>" >> ${RESULTS}
echo "********************** Creating a CA Certificate **********************"
echo nss > ${PWFILE}
echo "   certutil -N -d ${CADIR} -f ${PWFILE} redir ${CERTUTILOUT}" 
rm ${CERTUTILOUT} 2>/dev/null
echo "   certutil -N -d ${CADIR} -f ${PWFILE} redir ${CERTUTILOUT}"  >${CERTUTILOUT}
certutil -N -d ${CADIR} -f ${PWFILE} >>${CERTUTILOUT} 2>&1

echo initialized
echo 5 > ${CERTSCRIPT}
echo 9 >> ${CERTSCRIPT}
echo n >> ${CERTSCRIPT}
echo y >> ${CERTSCRIPT}
echo 3 >> ${CERTSCRIPT}
echo n >> ${CERTSCRIPT}
echo 5 >> ${CERTSCRIPT}
echo 6 >> ${CERTSCRIPT}
echo 7 >> ${CERTSCRIPT}
echo 9 >> ${CERTSCRIPT}
echo n >> ${CERTSCRIPT}
echo    "certutil -S -n \"TestCA\" -s \"CN=NSS Test CA, O=BOGUS NSS, L=Mountain View, ST=California, C=US\" -t \"CTu,CTu,CTu\" -v 60 -x -d ${CADIR} -1 -2 -5 -f ${PWFILE} -z ${NOISE_FILE} redir ${CERTUTILOUT}"
echo    "certutil -S -n \"TestCA\" -s \"CN=NSS Test CA, O=BOGUS NSS, L=Mountain View, ST=California, C=US\" -t \"CTu,CTu,CTu\" -v 60 -x -d ${CADIR} -1 -2 -5 -f ${PWFILE} -z ${NOISE_FILE} redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -S -n "TestCA" -s "CN=NSS Test CA, O=BOGUS NSS, L=Mountain View, ST=California, C=US" -t "CTu,CTu,CTu" -v 60 -x -d ${CADIR} -1 -2 -5 -f ${PWFILE} -z ${NOISE_FILE} < ${CERTSCRIPT} >>${CERTUTILOUT} 2>&1

if [ $? -ne 0 ]; then
    echo "<TR><TD>Creating CA Cert</TD><TD bgcolor=red>Failed</TD><TR>" >> ${RESULTS}
else
    echo "<TR><TD>Creating CA Cert</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi
echo "   certutil -L -n \"TestCA\" -r -d ${CADIR} > root.cert"
echo "   certutil -L -n \"TestCA\" -r -d ${CADIR} > root.cert" >>${CERTUTILOUT}
certutil -L -n "TestCA" -r -d ${CADIR} > root.cert 2>>${CERTUTILOUT}
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Export Root"}
fi

echo "**************** Creating Client CA Issued Certificates ****************"
echo "   certutil -N -d ${ALICEDIR} -f ${PWFILE} redir $CERTUTILOUT"
echo "   certutil -N -d ${ALICEDIR} -f ${PWFILE} redir $CERTUTILOUT" >>${CERTUTILOUT}
certutil -N -d ${ALICEDIR} -f ${PWFILE} >>${CERTUTILOUT} 2>&1
netstat >> ${NOISE_FILE} 2>&1
date >> ${NOISE_FILE} 2>&1
cd ${ALICEDIR}
echo "Import the root CA"
echo "   certutil -A -n \"TestCA\" -t \"TC,TC,TC\" -f ${PWFILE} -d ${ALICEDIR} -i ${CADIR}/root.cert redir ${CERTUTILOUT}"
echo "   certutil -A -n \"TestCA\" -t \"TC,TC,TC\" -f ${PWFILE} -d ${ALICEDIR} -i ${CADIR}/root.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -A -n "TestCA" -t "TC,TC,TC" -f ${PWFILE} -d ${ALICEDIR} -i ${CADIR}/root.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Root"}
fi
echo "Generate a Certificate request"
echo  "  certutil -R -s \"CN=Alice, E=alice@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d ${ALICEDIR}  -f ${PWFILE} -z ${NOISE_FILE} -o req redir ${CERTUTILOUT}"
echo  "  certutil -R -s \"CN=Alice, E=alice@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d ${ALICEDIR}  -f ${PWFILE} -z ${NOISE_FILE} -o req redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -R -s "CN=Alice, E=alice@bogus.com, O=BOGUS NSS, L=Mountain View, ST=California, C=US" -d ${ALICEDIR}  -f ${PWFILE} -z ${NOISE_FILE} -o req >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Generate Request"}
fi
echo "Sign the Certificate request"
echo  "certutil -C -c "TestCA" -m 3 -v 60 -d ${CADIR} -f ${PWFILE} -i req -o alice.cert redir ${CERTUTILOUT}"
echo  "certutil -C -c "TestCA" -m 3 -v 60 -d ${CADIR} -f ${PWFILE} -i req -o alice.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -C -c "TestCA" -m 3 -v 60 -d ${CADIR} -i req -o alice.cert -f ${PWFILE} >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Sign Alice's Cert"}
fi
echo "Import the new Cert"
echo "certutil -A -n \"Alice\" -t \"u,u,u\" -d ${ALICEDIR} -f ${PWFILE} -i alice.cert redir ${CERTUTILOUT}"
echo "certutil -A -n \"Alice\" -t \"u,u,u\" -d ${ALICEDIR} -f ${PWFILE} -i alice.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -A -n "Alice" -t "u,u,u" -d ${ALICEDIR} -f ${PWFILE} -i alice.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Alice's cert"}
fi
if [ -n "${CERTFAILED}" ]; then
    echo "<TR><TD>Creating Alice's email cert</TD><TD bgcolor=red>Failed ($CERTFAILED)</TD><TR>" >> ${RESULTS}
else
    echo "<TR><TD>Creating Alice's email cert</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi

netstat >> ${NOISE_FILE} 2>&1
date >> ${NOISE_FILE} 2>&1
echo "certutil  -N -d ${BOBDIR} -f  redir ${CERTUTILOUT}"
echo "certutil  -N -d ${BOBDIR} -f  redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -N -d ${BOBDIR} -f ${PWFILE} >>${CERTUTILOUT} 2>&1
cd ${BOBDIR}
echo "Import the root CA"
echo "   certutil -A -n \"TestCA\" -t \"TC,TC,TC\" -f ${PWFILE} -d ${BOBDIR} -i ${CADIR}/root.cert redir ${CERTUTILOUT}"
echo "   certutil -A -n \"TestCA\" -t \"TC,TC,TC\" -f ${PWFILE} -d ${BOBDIR} -i ${CADIR}/root.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -A -n "TestCA" -t "TC,TC,TC" -f ${PWFILE} -d ${BOBDIR} -i ${CADIR}/root.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Root"}
fi
echo "Generate a Certificate request"
echo  "  certutil -R -s \"CN=Bob, E=bob@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d ${BOBDIR}  -f ${PWFILE} -z ${NOISE_FILE} -o req redir ${CERTUTILOUT}"
echo  "  certutil -R -s \"CN=Bob, E=bob@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d ${BOBDIR}  -f ${PWFILE} -z ${NOISE_FILE} -o req redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -R -s "CN=Bob, E=bob@bogus.com, O=BOGUS NSS, L=Mountain View, ST=California, C=US" -d ${BOBDIR}  -f ${PWFILE} -z ${NOISE_FILE} -o req >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Generate Request"}
fi
echo "Sign the Certificate request"
echo  "certutil -C -c "TestCA" -m 4 -v 60 -d ${CADIR} -f ${PWFILE} -i req -o bob.cert redir ${CERTUTILOUT}"
echo  "certutil -C -c "TestCA" -m 4 -v 60 -d ${CADIR} -f ${PWFILE} -i req -o bob.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -C -c "TestCA" -m 4 -v 60 -d ${CADIR} -i req -o bob.cert -f ${PWFILE} >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Sign Bob's cert"}
fi
echo "Import the new Cert"
echo "certutil -A -n \"Bob\" -t \"u,u,u\" -d ${BOBDIR} -f ${PWFILE} -i bob.cert redir ${CERTUTILOUT}"
echo "certutil -A -n \"Bob\" -t \"u,u,u\" -d ${BOBDIR} -f ${PWFILE} -i bob.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -A -n "Bob" -t "u,u,u" -d ${BOBDIR} -f ${PWFILE} -i bob.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Bob's cert"}
fi
if [ -n "${CERTFAILED}" ]; then
    echo "<TR><TD>Creating Bob's email cert</TD><TD bgcolor=red>Failed ($CERTFAILED)</TD><TR>" >> ${RESULTS}
else
    echo "<TR><TD>Creating Bob's email cert</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi

netstat >> ${NOISE_FILE} 2>&1
date >> ${NOISE_FILE} 2>&1
cd ${CADIR}
echo "Generate a third cert"
echo "certutil -S -n \"Dave\" -c \"TestCA\" -t \"u,u,u\" -s \"CN=Dave, E=dave@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d ${CADIR} -f ${PWFILE} -z ${NOISE_FILE} -m 5 -v 60 redir ${CERTUTILOUT}"
echo "certutil -S -n \"Dave\" -c \"TestCA\" -t \"u,u,u\" -s \"CN=Dave, E=dave@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US\" -d ${CADIR} -f ${PWFILE} -z ${NOISE_FILE} -m 5 -v 60 redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -S -n "Dave" -c "TestCA" -t "u,u,u" -s "CN=Dave, E=dave@bogus.com, O=BOGUS Netscape, L=Mountain View, ST=California, C=US" -d ${CADIR} -f ${PWFILE} -z ${NOISE_FILE} -m 5 -v 60 >>${CERTUTILOUT} 2>&1

echo "Import Alices's cert into Bob's db"
echo "certutil -E -t \"u,u,u\" -d ${BOBDIR} -f ${PWFILE} -i ${ALICEDIR}/alice.cert redir ${CERTUTILOUT}"
echo "certutil -E -t \"u,u,u\" -d ${BOBDIR} -f ${PWFILE} -i ${ALICEDIR}/alice.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -E -t "u,u,u" -d ${BOBDIR} -f ${PWFILE} -i ${ALICEDIR}/alice.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Alice's cert into Bob's db"}
fi
echo "Import Bob's cert into Alice's db"
echo "certutil -E -t \"u,u,u\" -d ${ALICEDIR} -f ${PWFILE} -i ${BOBDIR}/bob.cert redir ${CERTUTILOUT}"
echo "certutil -E -t \"u,u,u\" -d ${ALICEDIR} -f ${PWFILE} -i ${BOBDIR}/bob.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -E -t "u,u,u" -d ${ALICEDIR} -f ${PWFILE} -i ${BOBDIR}/bob.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Bob's cert into Alice's db"}
fi
echo "Import Dave's cert into Alice's and Bob's dbs"
echo "   certutil -L -n \"Dave\" -r -d ${CADIR} > dave.cert"
echo "   certutil -L -n \"Dave\" -r -d ${CADIR} > dave.cert" >>${CERTUTILOUT}
certutil -L -n "Dave" -r -d ${CADIR} > dave.cert 2>>${CERTUTILOUT}
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Export Dave's cert"}
fi
echo "certutil -E -t \"u,u,u\" -d ${ALICEDIR} -f ${PWFILE} -i ${CADIR}/dave.cert redir ${CERTUTILOUT}"
echo "certutil -E -t \"u,u,u\" -d ${ALICEDIR} -f ${PWFILE} -i ${CADIR}/dave.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -E -t "u,u,u" -d ${ALICEDIR} -f ${PWFILE} -i ${CADIR}/dave.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Dave's cert into Alice's db"}
fi
echo "certutil -E -t \"u,u,u\" -d ${BOBDIR} -f ${PWFILE} -i ${CADIR}/dave.cert redir ${CERTUTILOUT}"
echo "certutil -E -t \"u,u,u\" -d ${BOBDIR} -f ${PWFILE} -i ${CADIR}/dave.cert redir ${CERTUTILOUT}" >>${CERTUTILOUT}
certutil -E -t "u,u,u" -d ${BOBDIR} -f ${PWFILE} -i ${CADIR}/dave.cert >>${CERTUTILOUT} 2>&1
if [ $? -ne 0 ]; then
   CERTFAILED=${CERTFAILED-"Import Dave's cert into Bob's db"}
fi
echo "</TABLE><BR>" >> ${RESULTS}

echo "********************* S/MIME testing  ****************************"
echo "<TABLE BORDER=1><TR><TH COLSPAN=3>S/MIME tests</TH></TR>" >> ${RESULTS}
echo "<TR><TH width=500>Test Case</TH><TH width=50>Result</TH></TR>" >> ${RESULTS}
cd ${SMIMEDIR}
cp ${CURDIR}/alice.txt ${SMIMEDIR}
# hopefully fixes HP problem where crashes first time cmsutil is run
cmsutil < alice.txt
# Test basic signed and enveloped messages from 1 --> 2
echo "cmsutil -S -N Alice -i alice.txt -d ${ALICEDIR} -p nss -o alice.sig"
cmsutil -S -N Alice -i alice.txt -d ${ALICEDIR} -p nss -o alice.sig
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Create Signature Alice"}
fi
echo "cmsutil -D -i alice.sig -d ${BOBDIR} -o alice.data1"
cmsutil -D -i alice.sig -d ${BOBDIR} -o alice.data1
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Decode Alice's Signature"}
fi
echo "diff alice.txt alice.data1"
diff alice.txt alice.data1
if [ $? -ne 0 ]; then
   echo "Signing attached message Failed ($CMSFAILED)" 
   echo "<TR><TD>Signing attached message</TD><TD bgcolor=red>Failed ($CMSFAILED)</TD><TR>" >> ${RESULTS}
else
   echo "Signing attached message Passed" 
   echo "<TR><TD>Signing attached message</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi
echo "cmsutil -E -r bob@bogus.com -i alice.txt -d ${ALICEDIR} -p nss -o alice.env"
cmsutil -E -r bob@bogus.com -i alice.txt -d ${ALICEDIR} -p nss -o alice.env
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Create Enveloped Data Alice"}
fi
echo "cmsutil -D -i alice.env -d ${BOBDIR} -p nss -o alice.data1"
cmsutil -D -i alice.env -d ${BOBDIR} -p nss -o alice.data1
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Decode Enveloped Data Alice"}
fi
echo "diff alice.txt alice.data1"
diff alice.txt alice.data1
if [ $? -ne 0 ]; then
   echo "Enveloped Data Failed ($CMSFAILED)" 
   echo "<TR><TD>Enveloped Data</TD><TD bgcolor=red>Failed ($CMSFAILED)</TD><TR>" >> ${RESULTS}
else
   echo "Enveloped Data Passed"
   echo "<TR><TD>Enveloped Data</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi
# multiple recip
#cmsutil -E -i alicecc.txt -d alicedir -o alicecc.env -r bob@bogus.com,dave@bogus.com
#cmsutil -D -i alicecc.env -d bobdir -p nss

#certs-only
echo "cmsutil -O -r \"Alice,bob@bogus.com,dave@bogus.com\" -d ${ALICEDIR} > co.der"
cmsutil -O -r "Alice,bob@bogus.com,dave@bogus.com" -d ${ALICEDIR} < alice.txt > co.der
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Create Certs-Only Alice"}
fi
echo "cmsutil -D -i co.der -d ${BOBDIR}"
cmsutil -D -i co.der -d ${BOBDIR}
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Verify Certs-Only by CA"}
fi
if [ -n "${CMSFAILED}" ]; then
    echo "Sending certs-only message Failed ($CMSFAILED)"
    echo "<TR><TD>Sending certs-only message</TD><TD bgcolor=red>Failed ($CMSFAILED)</TD><TR>" >> ${RESULTS}
else
    echo "Sending certs-only message Passed"
    echo "<TR><TD>Sending certs-only message</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi
echo "cmsutil -C -i alice.txt -e alicehello.env -d ${ALICEDIR} -r \"bob@bogus.com\" > alice.enc"
cmsutil -C -i alice.txt -e alicehello.env -d ${ALICEDIR} -r "bob@bogus.com" > alice.enc
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Create Encrypted-Data"}
fi
#echo "cmsutil -C -i bob.txt -e alicehello.env -d ${ALICEDIR} -r \"alice@bogus.com\" > bob.enc"
#cmsutil -C -i bob.txt -e alicehello.env -d ${ALICEDIR} -r "alice@bogus.com" > bob.enc
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Create Encrypted-Data"}
fi
echo "cmsutil -D -i alice.enc -d ${BOBDIR} -e alicehello.env -p nss -o alice.data2"
cmsutil -D -i alice.enc -d ${BOBDIR} -e alicehello.env -p nss -o alice.data2
diff alice.txt alice.data2
if [ $? -ne 0 ]; then
   CMSFAILED=${CMSFAILED-"Decode Encrypted-Data"}
fi
if [ -n "${CMSFAILED}" ]; then
    echo "Encrypted-Data message Failed ($CMSFAILED)"
    echo "<TR><TD>Encrypted-Data message</TD><TD bgcolor=red>Failed ($CMSFAILED)</TD><TR>" >> ${RESULTS}
else
    echo "Encrypted-Data message Passed"
    echo "<TR><TD>Encrypted-Data message</TD><TD bgcolor=lightGreen>Passed</TD><TR>" >> ${RESULTS}
fi

echo "</TABLE><BR>" >> ${RESULTS}

if [ "$SONMI_DEBUG" != "ON"  -a -n "$TEMPFILES" ]
then
	rm -f ${TEMPFILES}
fi
cd ${CURDIR}

echo "</BODY></HTML>" >> ${RESULTS}
