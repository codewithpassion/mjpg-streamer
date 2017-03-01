#!/bin/bash
set -e

while getopts c: opt
do
    case "$opt" in
      c)  DEB_CODENAME="$OPTARG";;
      \?)		# unknown flag
      	  echo >&2 \
	  "usage: $0 [-c deb_codename]"
	  exit 1;;
    esac
done
shift `expr $OPTIND - 1`

if [ "$DEB_CODENAME" = "" ]; then
        echo "Please set the DEB_CODENAME environment variable to define into what debian repo we should upload the .deb files."
        exit 1
fi

bucket="openrov-software-nightlies"
folder="/${DEB_CODENAME}/mjpeg-streamer"

contentType="application/x-compressed-tar"
dateValue=`date -R`
for f in *.deb; do
resource="/${bucket}${folder}/${f}"
stringToSign="PUT\n\n${contentType}\n${dateValue}\n${resource}"

signature=`echo -en ${stringToSign} | openssl sha1 -hmac ${s3Secret} -binary | base64`


echo "publishing https://${bucket}.s3.amazonaws.com${folder}/${f}"
set +x
curl -X PUT -T "${f}" \
  -H "Host: ${bucket}.s3.amazonaws.com" \
  -H "Date: ${dateValue}" \
  -H "Content-Type: ${contentType}" \
  -H "Authorization: AWS ${s3Key}:${signature}" \
  https://${bucket}.s3.amazonaws.com${folder}/${f}; done
