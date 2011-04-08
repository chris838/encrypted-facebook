import random
import string
import time
from Crypto.Cipher import AES

# Generate key
key = ''.join(random.choice(string.letters) for i in xrange(16))

obj=AES.new(key, AES.MODE_ECB)

plain=''.join(random.choice(string.letters) for i in xrange(16*10000 -1))

start = time.time()

for i in range(1,1000):
    ciph=obj.encrypt( plain+str(i%10) )
    ciph=obj.decrypt( ciph )

elapsed = (time.time() - start)

print elapsed