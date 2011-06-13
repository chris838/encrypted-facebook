# /usr/bin/bash
rm *.tfm *.fd
rename 'y/A-Z/a-z/' *.ttf 
rename 's/ /_/g' *.ttf
for file in *.ttf
do
	fn=${file/.ttf/}
	ttf2tfm $file -p t1-wgl4.enc
	cp t1lhandw.fd.TEMPLATE t1${fn}.fd.TEST
	sed -e "s/lhandw/${fn}/g" t1${fn}.fd.TEST > t1${fn}.fd
done
rm *.TEST
