from example import *
#print myadd(1,2)

with open("person.pb", "r") as fh:
    content = fh.read()
sp = sproto_create(content)
st = sproto_type(sp, "Person");
result = sproto_encode(st, {
    "name": "John",
    "id":1001,
    "email":"crystal@example.com",
    "phone":[
        {
            "type" : 1,
            "number": "10086",
        },
    ],
    })

print "-------------------------"
print sproto_decode(st, result)
