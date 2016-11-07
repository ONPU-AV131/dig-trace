#!/usr/bin/python
import dns.name
import dns.resolver
import sys
len(sys.argv)
domain=str(sys.argv[1])
domains= domain.split('.',domain.count('.'))
domains[len(domains)-1]+='.'
#domain='google.com'
check =domains[len(domains)-1] 
for data in reversed(domains): 
    if not data[-1:]=='.':
        check = data+'.' + check
    nameservers=dns.resolver.query(check,'NS')
    for dataNS in nameservers:
        print check ,' ',dataNS
