#ifndef STRATIFIER_INTERNAL_H
#define STRATIFIER_INTERNAL_H

#include "libckpool.h"
#include "uthash.h"
#include "utlist.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define UA_TRUNCATE_LEN 64

/* Forward declarations used by these structs */
struct userwb;
typedef struct stratum_instance stratum_instance_t;
typedef struct user_instance user_instance_t;
typedef struct worker_instance worker_instance_t;
typedef struct stratifier_data sdata_t;
typedef struct proxy_base proxy_t;
typedef struct ckpool_instance ckpool_t;

/* Struct definitions used by stratifier and tests */

/* Combined data from users */
struct user_instance {
	UT_hash_handle hh;
	char username[128];
	int id;
	char *secondaryuserid;
	bool btcaddress;
	bool script;
	bool segwit;

	/* A linked list of all connected instances of this user */
	struct stratum_instance *clients;

	/* A linked list of all connected workers of this user */
	struct worker_instance *worker_instances;

	int workers;
	int remote_workers;
	char txnbin[48];
	int txnlen;
	struct userwb *userwbs; /* Protected by instance lock */

	double best_diff; /* Best share found by this user */
	double best_ever; /* Best share ever found by this user */

	double shares;

	double uadiff; /* Shares not yet accounted for in hashmeter */

	double dsps1; /* Diff shares per second, 1 minute rolling average */
	double dsps15s; /* ... 15 second ... */
	double dsps5; /* ... 5 minute ... */
	double dsps60;/* etc */
	double dsps1440;
	double dsps10080;
	tv_t last_share;
	tv_t last_decay;

	bool authorised; /* Has this username ever been authorised? */
	time_t auth_time;
	time_t failed_authtime; /* Last time this username failed to authorise */
	int auth_backoff; /* How long to reject any auth attempts since last failure */
	bool throttled; /* Have we begun rejecting auth attempts */
};

/* Combined data from workers with the same workername */
struct worker_instance {
	struct user_instance *user_instance;
	char *workername;
	/* last-seen user agent string for this worker (persisted in user JSON) */
	char *useragent;
	char norm_useragent[UA_TRUNCATE_LEN + 1];

	/* Number of stratum instances attached as this one worker */
	int instance_count;

	struct worker_instance *next;
	struct worker_instance *prev;

	double shares;

	double uadiff; /* Shares not yet accounted for in hashmeter */

	double dsps1;
	double dsps15s;
	double dsps5;
	double dsps60;
	double dsps1440;
	double dsps10080;
	tv_t last_share;
	tv_t last_decay;
	time_t start_time;
	time_t last_connect;

	double best_diff; /* Best share found by this worker */
	double best_ever; /* Best share ever found by this worker */
	double mindiff; /* User chosen mindiff */

	bool idle;
	bool notified_idle;
};

/* Per client stratum instance == workers */
struct stratum_instance {
	UT_hash_handle hh;
	int64_t id;

	/* Virtualid used as unique local id for passthrough clients */
	int64_t virtualid;

	struct stratum_instance *recycled_next;
	struct stratum_instance *recycled_prev;

	struct stratum_instance *user_next;
	struct stratum_instance *user_prev;

	struct stratum_instance *node_next;
	struct stratum_instance *node_prev;

	struct stratum_instance *remote_next;
	struct stratum_instance *remote_prev;

	/* Descriptive of ID number and passthrough if any */
	char identity[128];

	/* Reference count for when this instance is used outside of the
	 * instance_lock */
	int ref;

	char enonce1[36]; /* Fit up to 16 byte binary enonce1 */
	uchar enonce1bin[16];
	char enonce1var[20]; /* Fit up to 8 byte binary enonce1var */
	uint64_t enonce1_64;
	int session_id;

	double diff; /* Current diff */
	double old_diff; /* Previous diff */
	int64_t diff_change_job_id; /* Last job_id we changed diff */

	double uadiff; /* Shares not yet accounted for in hashmeter */

	double dsps1; /* Diff shares per second, 1 minute rolling average */
	double dsps15s; /* ... 15 second ... */
	double dsps5; /* ... 5 minute ... */
	double dsps60;/* etc */
	double dsps1440;
	double dsps10080;
	tv_t ldc; /* Last diff change */
	double ssdc; /* Shares since diff change */
	tv_t first_share;
	tv_t last_share;
	tv_t last_decay;
	time_t first_invalid; /* Time of first invalid in run of non stale rejects */
	time_t upstream_invalid; /* As first_invalid but for upstream responses */
	time_t start_time;

	char address[INET6_ADDRSTRLEN];
	bool node; /* Is this a mining node */
	bool subscribed;
	bool authorising; /* In progress, protected by instance_lock */
	bool authorised;
	bool dropped;
	bool idle;
	int reject;	/* Indicator that this client is having a run of rejects
			 * or other problem and should be dropped lazily if
			 * this is set to 2 */

	int latency; /* Latency when on a mining node */

	bool reconnect; /* This client really needs to reconnect */
	time_t reconnect_request; /* The time we sent a reconnect message */

	user_instance_t *user_instance;
	worker_instance_t *worker_instance;

	char *useragent;
	char *workername;
	char *password;
	bool messages; /* Is this a client that understands stratum messages */
	int user_id;
	int server; /* Which server is this instance bound to */

	ckpool_t *ckp;

	time_t last_txns; /* Last time this worker requested txn hashes */
	time_t disconnected_time; /* Time this instance disconnected */

	double suggest_diff; /* Stratum client suggested diff */
	double best_diff; /* Best share found by this instance */
	bool password_diff_set; /* Was diff set via password field? Preferred over stratum suggest */

	sdata_t *sdata; /* Which sdata this client is bound to */
	proxy_t *proxy; /* Proxy this is bound to in proxy mode */
	int proxyid; /* Which proxy id  */
	int subproxyid; /* Which subproxy */

	bool passthrough; /* Is this a passthrough */
	bool trusted; /* Is this a trusted remote server */
	bool remote; /* Is this a remote client on a trusted remote server */
};

#endif /* STRATIFIER_INTERNAL_H */
