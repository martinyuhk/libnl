/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef __NL_TEST_UTIL_H__
#define __NL_TEST_UTIL_H__

#include <errno.h>
#include <sys/stat.h>
#include <check.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "netlink/object.h"
#include "netlink/cache.h"

#include "netlink-private/nl-auto.h"
#include "netlink-private/utils.h"

/*****************************************************************************/

static inline void _nltst_strfreev(char **strv)
{
	size_t i;

	if (strv) {
		for (i = 0; strv[i]; i++)
			free(strv[i]);
		free(strv);
	}
}

#define _nltst_auto_strfreev _nl_auto(_nltst_auto_strfreev_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(char **, _nltst_auto_strfreev_fcn, _nltst_strfreev);

/*****************************************************************************/

#ifndef ck_assert_ptr_nonnull
#define ck_assert_ptr_nonnull(ptr) ck_assert(ptr)
#endif

/*****************************************************************************/

void _nltst_get_urandom(void *ptr, size_t len);

uint32_t _nltst_rand_u32(void);

static inline uint32_t _nltst_rand_bool(void)
{
	return _nltst_rand_u32() % 2 == 0;
}

/*****************************************************************************/

#define _nltst_assert(expr)                                                    \
	({                                                                     \
		typeof(expr) _expr = (expr);                                   \
                                                                               \
		if (!_expr) {                                                  \
			ck_assert_msg(0, "assert(%s) failed", #expr);          \
		}                                                              \
		_expr;                                                         \
	})

#define _nltst_assert_errno(expr)                                              \
	do {                                                                   \
		if (expr) {                                                    \
		} else {                                                       \
			const int _errno = (errno);                            \
                                                                               \
			ck_assert_msg(0, "assert(%s) failed (errno=%d, %s)",   \
				      #expr, _errno, strerror(_errno));        \
		}                                                              \
	} while (0)

#define _nltst_assert_retcode(expr)                                                   \
	do {                                                                          \
		const int _r = (expr);                                                \
                                                                                      \
		if (_r < 0) {                                                         \
			ck_assert_msg(                                                \
				0, "command(%s) failed with return code %d",          \
				#expr, _r);                                           \
		}                                                                     \
		if (_r > 0) {                                                         \
			ck_assert_msg(                                                \
				0,                                                    \
				"command(%s) has unexpected positive return code %d", \
				#expr, _r);                                           \
		}                                                                     \
	} while (0)

#define _nltst_close(fd)                                                       \
	do {                                                                   \
		int _r;                                                        \
                                                                               \
		_r = _nl_close((fd));                                          \
		_nltst_assert_errno(_r == 0);                                  \
	} while (0)

#define _nltst_fclose(f)                                                       \
	do {                                                                   \
		int _r;                                                        \
                                                                               \
		_r = fclose((f));                                              \
		_nltst_assert_errno(_r == 0);                                  \
	} while (0)

#define _nltst_assert_link_exists_full(ifname, exists)                                                       \
	do {                                                                                                 \
		const bool _exists = (exists);                                                               \
		const char *_ifname = (ifname);                                                              \
		char _path[100];                                                                             \
		struct stat _stat;                                                                           \
		int _r;                                                                                      \
                                                                                                             \
		ck_assert_pstr_ne(_ifname, NULL);                                                            \
		ck_assert_int_lt(strlen(_ifname), IFNAMSIZ);                                                 \
                                                                                                             \
		strcpy(_path, "/sys/class/net/");                                                            \
		strcat(_path, _ifname);                                                                      \
		ck_assert_int_lt(strlen(_path), sizeof(_path));                                              \
                                                                                                             \
		_r = stat(_path, &_stat);                                                                    \
		if (_exists) {                                                                               \
			if (_r != 0) {                                                                       \
				const int _errsv = (errno);                                                  \
                                                                                                             \
				ck_assert_msg(                                                               \
					0,                                                                   \
					"link(%s) does not exist (stat(%s) failed with r=%d, errno=%d (%s)", \
					_ifname, _path, _r, _errsv,                                          \
					strerror(_errsv));                                                   \
			}                                                                                    \
		} else {                                                                                     \
			if (_r != -1 && errno != ENOENT) {                                                   \
				const int _errsv = (errno);                                                  \
                                                                                                             \
				ck_assert_msg(                                                               \
					0,                                                                   \
					"link(%s) should not exist but stat(%s) gave r=%d, errno=%d (%s)",   \
					_ifname, _path, _r, _errsv,                                          \
					strerror(_errsv));                                                   \
			}                                                                                    \
		}                                                                                            \
	} while (0);

#define _nltst_assert_link_exists(ifname)                                      \
	_nltst_assert_link_exists_full((ifname), true)

#define _nltst_assert_link_not_exists(ifname)                                  \
	_nltst_assert_link_exists_full((ifname), false)

/*****************************************************************************/

typedef union {
	in_addr_t addr4;
	struct in_addr a4;
	struct in6_addr a6;
} NLTstIPAddr;

static inline char *_nltst_inet_ntop(int addr_family, const void *addr,
				     char buf[static INET_ADDRSTRLEN])
{
	char *r;

	ck_assert(addr_family == AF_INET || addr_family == AF_INET6);
	ck_assert(addr);

	r = (char *)inet_ntop(addr_family, addr, buf,
			      (addr_family == AF_INET) ? INET_ADDRSTRLEN :
							       INET6_ADDRSTRLEN);
	ck_assert_ptr_eq(r, buf);
	ck_assert_int_lt(strlen(r), (addr_family == AF_INET) ?
					    INET_ADDRSTRLEN :
						  INET6_ADDRSTRLEN);
	return r;
}

static inline char *_nltst_inet_ntop_dup(int addr_family, const void *addr)
{
	return (char *)_nltst_inet_ntop(addr_family, addr,
					malloc((addr_family == AF_INET) ?
						       INET_ADDRSTRLEN :
							     INET6_ADDRSTRLEN));
}

static inline bool _nltst_inet_pton(int addr_family, const char *str,
				    int *out_addr_family, void *out_addr)
{
	NLTstIPAddr a;
	int r;

	ck_assert(addr_family == AF_UNSPEC || addr_family == AF_INET ||
		  addr_family == AF_INET6);

	/* when requesting @out_addr, then the addr-family must either be
	 * pre-determined or requested too. */
	ck_assert(!out_addr || out_addr_family || addr_family != AF_UNSPEC);

	if (!str)
		return false;

	if (addr_family == AF_UNSPEC)
		addr_family = strchr(str, ':') ? AF_INET6 : AF_INET;

	r = inet_pton(addr_family, str, &a);
	if (r != 1)
		return false;

	if (out_addr) {
		memcpy(out_addr, &a,
		       addr_family == AF_INET ? sizeof(in_addr_t) :
						      sizeof(struct in6_addr));
	}
	if (out_addr_family)
		*out_addr_family = addr_family;

	return true;
}

static inline bool _nltst_inet_valid(int addr_family, const char *addr)
{
	return _nltst_inet_pton(addr_family, addr, NULL, NULL);
}

static inline int _nltst_inet_addr_family(int addr_family, const char *addr)
{
	if (!_nltst_inet_pton(addr_family, addr, &addr_family, NULL))
		return AF_UNSPEC;
	return addr_family;
}

static inline char *_nltst_inet_normalize(int addr_family, const char *addr,
					  char buf[static INET_ADDRSTRLEN])
{
	NLTstIPAddr a;

	buf[0] = '\0';
	if (!_nltst_inet_pton(addr_family, addr, &addr_family, &a))
		return NULL;
	return _nltst_inet_ntop(addr_family, &a, buf);
}

/*****************************************************************************/

char *_nltst_strtok(const char **p_str);

char **_nltst_strtokv(const char *str);

#define _nltst_assert_strv_equal(strv1, strv2)                                 \
	do {                                                                   \
		typeof(strv1) _strv1 = (strv1);                                \
		typeof(strv2) _strv2 = (strv2);                                \
		_nl_unused const void *_strv1_typecheck1 = _strv1;             \
		_nl_unused const void *_strv2_typecheck1 = _strv2;             \
		_nl_unused const char *_strv1_typecheck2 =                     \
			_strv1 ? _strv1[0] : NULL;                             \
		_nl_unused const char *_strv2_typecheck2 =                     \
			_strv2 ? _strv2[0] : NULL;                             \
		size_t _i;                                                     \
                                                                               \
		ck_assert_int_eq(!!_strv1, !!_strv2);                          \
		if (_strv1) {                                                  \
			for (_i = 0; _strv1[_i] || _strv2[_i]; _i++) {         \
				ck_assert_str_eq(_strv1[_i], _strv2[_i]);      \
			}                                                      \
		}                                                              \
	} while (0)

#define _NLTST_CHARSET_SPACE " \n\r\t"

#define _nltst_char_is(ch, charset) (!!(strchr("" charset "", (ch))))

#define _nltst_char_is_space(ch) _nltst_char_is(ch, _NLTST_CHARSET_SPACE)

#define _nltst_str_skip_predicate(s, ch, predicate)                            \
	({                                                                     \
		typeof(s) _s1 = (s);                                           \
		_nl_unused const char *_s1_typecheck = (_s1);                  \
                                                                               \
		if (_s1) {                                                     \
			while (({                                              \
				const char ch = _s1[0];                        \
                                                                               \
				(ch != '\0') && (predicate);                   \
			}))                                                    \
				_s1++;                                         \
		}                                                              \
		_s1;                                                           \
	})

#define _nltst_str_skip_charset(s, charset)                                    \
	_nltst_str_skip_predicate(s, _ch, _nltst_char_is(_ch, "" charset ""))

#define _nltst_str_skip_space(s)                                               \
	_nltst_str_skip_charset(s, _NLTST_CHARSET_SPACE)

#define _nltst_str_has_prefix_and_space(s, prefix)                             \
	({                                                                     \
		typeof(s) _s2 = (s);                                           \
		_nl_unused const char *_s2_typecheck = (_s2);                  \
		const size_t _l = strlen("" prefix "");                        \
                                                                               \
		if (_s2) {                                                     \
			if ((strncmp(_s2, "" prefix "", _l)) == 0 &&           \
			    _nltst_char_is_space(_s2[_l]))                     \
				_s2 = _nltst_str_skip_space(&_s2[_l + 1]);     \
			else                                                   \
				_s2 = NULL;                                    \
		}                                                              \
		_s2;                                                           \
	})

#define _nltst_str_find_first_not_from_charset(s, charset)                     \
	({                                                                     \
		typeof(s) _s3 = (s);                                           \
		_nl_unused const char *_s3_typecheck = (_s3);                  \
		size_t _l3;                                                    \
                                                                               \
		_l3 = strspn(_s3, "" charset "");                              \
                                                                               \
		&_s3[_l3];                                                     \
	})

#define _nltst_str_find_first_from_charset(s, charset)                         \
	({                                                                     \
		typeof(s) _s3 = (s);                                           \
		_nl_unused const char *_s3_typecheck = (_s3);                  \
		size_t _l3;                                                    \
                                                                               \
		_l3 = strcspn(_s3, "" charset "");                             \
                                                                               \
		&_s3[_l3];                                                     \
	})

/*****************************************************************************/

void nltst_netns_fixture_setup(void);
void nltst_netns_fixture_teardown(void);

struct nltst_netns;

struct nltst_netns *nltst_netns_enter(void);
void nltst_netns_leave(struct nltst_netns *nsdata);

/*****************************************************************************/

typedef struct {
	int addr_family;
	int ifindex;
	int plen;
	char *addr;
	char *addr_pattern;
} NLTstSelectRoute;

#define _nltst_assert_select_route(select_route)                               \
	do {                                                                   \
		const NLTstSelectRoute *_select_route = (select_route);        \
                                                                               \
		ck_assert_ptr_nonnull(_select_route);                          \
		_nl_assert_addr_family_or_unspec(_select_route->addr_family);  \
		ck_assert_int_ge(_select_route->ifindex, 0);                   \
		ck_assert_int_ge(_select_route->plen, -1);                     \
		ck_assert_int_le(_select_route->plen,                          \
				 _select_route->addr_family == AF_INET ? 32 :  \
									       128); \
		ck_assert(!_select_route->addr || ({                           \
			char _buf[INET6_ADDRSTRLEN];                           \
			const char *_s2;                                       \
                                                                               \
			_s2 = _nltst_inet_normalize(                           \
				_select_route->addr_family,                    \
				_select_route->addr, _buf);                    \
			(_select_route->addr_family != AF_UNSPEC && _s2 &&     \
			 _nl_streq(_s2, _select_route->addr));                 \
		}));                                                           \
		ck_assert(!_select_route->addr_pattern ||                      \
			  !_select_route->addr);                               \
		ck_assert(!_select_route->addr_pattern ||                      \
			  _select_route->addr_family != AF_UNSPEC);            \
	} while (0)

void _nltst_select_route_clear(NLTstSelectRoute *select_route);

#define _nltst_auto_clear_select_route                                         \
	_nl_auto(_nltst_auto_clear_select_route_fcn)
_NL_AUTO_DEFINE_FCN_STRUCT(NLTstSelectRoute, _nltst_auto_clear_select_route_fcn,
			   _nltst_select_route_clear);

bool _nltst_select_route(struct nl_object *route,
			 const NLTstSelectRoute *selector);

void _nltst_expected_routes_parse(const char *str,
				  NLTstSelectRoute *out_select_route);

void _nltst_object_identical(const void *a, const void *b);

char *_nltst_object_to_string(struct nl_object *obj);

struct nl_object **_nltst_cache_get_all(struct nl_cache *cache,
					size_t *out_len);

struct nl_cache *_nltst_rtnl_link_alloc_cache(struct nl_sock *sk,
					      int addr_family, unsigned flags);

struct nl_cache *_nltst_rtnl_route_alloc_cache(struct nl_sock *sk,
					       int addr_family);

struct nl_sock *_nltst_socket(int protocol);

void _nltst_add_link(struct nl_sock *sk, const char *ifname, const char *kind);

void _nltst_delete_link(struct nl_sock *sk, const char *ifname);

void _nltst_assert_route_list(struct nl_object *const *objs, ssize_t len,
			      const char *const *expected_routes);

void _nltst_assert_route_cache_v(struct nl_cache *cache,
				 const char *const *expected_routes);

#define _nltst_assert_route_cache(cache, ...)                                  \
	_nltst_assert_route_cache_v(                                           \
		cache, ((const char *const[]){ __VA_ARGS__, NULL }))

#endif /* __NL_TEST_UTIL_H__ */
