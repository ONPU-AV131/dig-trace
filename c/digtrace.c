#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>

void
get_nsaddr_list(struct __res_state *in_state, unsigned char *nsbuf) {
	ns_msg msg;
    ns_rr rr;
	int i, l;

	ns_initparse(nsbuf, PACKETSZ, &msg);
	in_state->nscount = 0;
    l = ns_msg_count(msg, ns_s_ar);
	for (i = 0; i < l; i++) {
		ns_parserr(&msg, ns_s_ar, i, &rr);

		if (ns_rr_class(rr) == ns_c_in && ns_rr_type(rr) == ns_t_a) {
			if (in_state->nscount)
				printf(", ");
			printf("%s=%s", ns_rr_name(rr), inet_ntoa(*(struct in_addr*) ns_rr_rdata(rr)));

			in_state->nscount++;
			in_state->nsaddr_list[i].sin_addr = *(struct in_addr*) ns_rr_rdata(rr);

			if (in_state->nscount == MAXNS)
				return;
		}
	}
}

void
next_ns(unsigned char *nsbuf, char *dname, int depth) {
    unsigned char buf[PACKETSZ];
	struct __res_state mystate;
	ns_msg msg;
    ns_rr rr;
	int i, l;

	printf("%d. ", depth);
	res_ninit(&mystate);
	mystate.options &= ~RES_RECURSE;
	printf("(");
	get_nsaddr_list(&mystate, nsbuf);
	printf(")\n");


	l = res_nmkquery(&mystate, ns_o_query, dname, ns_c_in, ns_t_a, 0, 0, NULL, buf, sizeof(buf));
	l = res_nsend(&mystate, buf, sizeof(buf), nsbuf, PACKETSZ);

	if (l < 0) {
		herror("Can't send query");
		return;
	}

	ns_initparse(nsbuf, PACKETSZ, &msg);
    l = ns_msg_count(msg, ns_s_an);
	for (i = 0; i < l; i++) {
		ns_parserr(&msg, ns_s_an, i, &rr);
		if (
				ns_rr_class(rr) == ns_c_in && /* class is IN */
				ns_rr_type(rr) == ns_t_a &&   /* type  is A */
				strncmp(dname, ns_rr_name(rr), strlen(dname)) == 0 /* provided domain name is equal to the one in answer section */
		) {
			printf("RESULT: %s = %s\n", dname, inet_ntoa(*(struct in_addr*) ns_rr_rdata(rr)));
			return;
		}
	}

	next_ns(nsbuf, dname, depth + 1);
}

int
first_request(unsigned char *nsbuf) {
    unsigned char buf[PACKETSZ];
    int i, l;
	struct __res_state mystate;

	res_ninit(&mystate);

	mystate.options &= ~RES_RECURSE;
	l = res_nmkquery(&mystate, ns_o_query, ".", ns_c_in, ns_t_ns, 0, 0, NULL, buf, sizeof(buf));

	l = res_nsend(&mystate, buf, sizeof(buf), nsbuf, PACKETSZ);

	if (l < 0) {
		herror("Can't send query");
	}

	return l;
}

int
main(int argc, char **argv) {
	unsigned char nsbuf[PACKETSZ];

	if (first_request(nsbuf) < 0)
		return -1;

	if (argc > 1)
		next_ns(nsbuf, argv[1], 1);
	else
		next_ns(nsbuf, "betacoda.cono.org.ua", 1);

	return 0;
}
