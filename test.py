import math

#def calsum(start, end): 
    #return sum(range(start, end + 1))


#print(calsum(1, 10) + calsum(10, 20) + calsum(2, 30))

#n , s = 10, 23

#def fact(n):
#    s = 1
#    for i in range(1, n + 1):
#        s *= i
#    return s

#print(fact(n), s)

f = open("test.txt", encoding="utf-8")
r = f.readlines()
f.close()
for line in r:
    print(line.strip())
    #print(math.sqrt(2))