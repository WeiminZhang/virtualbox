/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)ip_output.c 8.3 (Berkeley) 1/21/94
 * ip_output.c,v 1.9 1994/11/16 10:17:10 jkh Exp
 */

/*
 * Changes and additions relating to SLiRP are
 * Copyright (c) 1995 Danny Gasparovski.
 *
 * Please read the file COPYRIGHT for the
 * terms and conditions of the copyright.
 */

#include <slirp.h>
#include "alias.h"

static int rt_lookup_in_cache(PNATState pData, uint32_t dst, uint8_t *ether)
{
    int rc = 1;
    rc = slirp_arp_lookup_ether_by_ip(pData, dst, ether);
    if (rc == 0)
        return rc;
    rc = bootp_cache_lookup_ether_by_ip(pData, dst, ether);
    if (rc == 0)
        return rc;
    /* 
     * no chance to send this packet, sorry, we will request ether address via ARP 
     */
    slirp_arp_who_has(pData, dst); 
    return rc; 
}

/*
 * IP output.  The packet in mbuf chain m contains a skeletal IP
 * header (with len, off, ttl, proto, tos, src, dst).
 * The mbuf chain containing the packet will be freed.
 * The mbuf opt, if present, will not be freed.
 */
int
ip_output(PNATState pData, struct socket *so, struct mbuf *m0)
{
    register struct ip *ip;
    register struct mbuf *m = m0;
    register int hlen = sizeof(struct ip );
    int len, off, error = 0;
    extern uint8_t zerro_ethaddr[ETH_ALEN];
    struct ethhdr *eh = NULL;
    uint8_t eth_dst[ETH_ALEN];
    int rc = 1;

    STAM_PROFILE_START(&pData->StatIP_output, a);

    DEBUG_CALL("ip_output");
    DEBUG_ARG("so = %lx", (long)so);
    DEBUG_ARG("m0 = %lx", (long)m0);

#ifndef VBOX_WITH_SLIRP_BSD_MBUF
    if(m->m_data != (MBUF_HEAD(m) + if_maxlinkhdr))
    {
        LogRel(("NAT: ethernet detects corruption of the packet"));
        AssertMsgFailed(("!!Ethernet frame corrupted!!"));
    }
#else
    M_ASSERTPKTHDR(m);
    Assert(m->m_pkthdr.header);
#endif

#if 0 /* We do no options */
    if (opt)
    {
        m = ip_insertoptions(m, opt, &len);
        hlen = len;
    }
#endif
#ifdef VBOX_WITH_SLIRP_BSD_MBUF
    M_ASSERTPKTHDR(m);
    Assert(m->m_pkthdr.header);
    ip = (struct ip *)m->m_pkthdr.header;
#else
    ip = mtod(m, struct ip *);
#endif
    /*
     * Fill in IP header.
     */
    ip->ip_v = IPVERSION;
    ip->ip_off &= IP_DF;
    ip->ip_id = htons(ip_currid++);
    ip->ip_hl = hlen >> 2;
    ipstat.ips_localout++;

    /*
     * Verify that we have any chance at all of being able to queue
     *      the packet or packet fragments
     */
#if 0 /* XXX Hmmm... */
    if (if_queued > if_thresh && towrite <= 0)
    {
        error = ENOBUFS;
        goto bad;
    }
#endif
    /* Current TCP/IP stack hasn't routing information at
     * all so we need to calculate destination ethernet address
     */
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
    eh = (struct ethhdr *)MBUF_HEAD(m);
    if (memcmp(eh->h_source, zerro_ethaddr, ETH_ALEN) == 0)
    {
       rc = rt_lookup_in_cache(pData, ip->ip_dst.s_addr, eth_dst);
       if (rc != 0)
           goto bad;
    }
    else
    {
       memcpy(eth_dst, eh->h_source, ETH_ALEN);
       rc = 0; /*some times we've already know where to send packet*/
    }
#else
    /* 
     * (vvl) Note: Here we should care about dhcp and other services
     * aware of ethernet address 
     */
    rc = rt_lookup_in_cache(pData, ip->ip_dst.s_addr, eth_dst);
    if (rc != 0)
        goto bad;
#endif
    /*
     * If small enough for interface, can just send directly.
     */
    if ((u_int16_t)ip->ip_len <= if_mtu)
    {
        ip->ip_len = htons((u_int16_t)ip->ip_len);
        ip->ip_off = htons((u_int16_t)ip->ip_off);
        ip->ip_sum = 0;
        ip->ip_sum = cksum(m, hlen);
    
        {
            STAM_PROFILE_START(&pData->StatALIAS_output, a);
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
            rc = LibAliasOut((m->m_la ? m->m_la : pData->proxy_alias), 
                mtod(m, char *), m->m_len);
            Log2(("NAT: LibAlias return %d\n", rc));
#else
            struct m_tag *t;
            if (t = m_tag_find(m, PACKET_TAG_ALIAS, NULL) != 0)
            {
                rc = LibAliasOut((struct libalias *)&t[1], mtod(m, char *), m_length(m, NULL));
            }
            else
            {
                rc = LibAliasOut(pData->proxy_alias, mtod(m, char *), 
                                 m_length(m, NULL));
            } 
            if (rc == PKT_ALIAS_IGNORED)
            {
                Log(("NAT: packet was droppped\n"));
                goto bad;
            }
#endif
            STAM_PROFILE_STOP(&pData->StatALIAS_output, a);
        }
#ifdef VBOX_WITH_SLIRP_BSD_MBUF
        /*todo: make prepend */
        m = m_prepend(pData, m, ETH_HLEN, M_DONTWAIT);
        Assert(m);
        eh = mtod(m, struct ethhdr *);
#endif
        memcpy(eh->h_source, eth_dst, ETH_ALEN); 

        if_output(pData, so, m);
        goto done;
     }

    /*
     * Too large for interface; fragment if possible.
     * Must be able to put at least 8 bytes per fragment.
     */
    if (ip->ip_off & IP_DF)
    {
        error = -1;
        ipstat.ips_cantfrag++;
        goto bad;
    }

    len = (if_mtu - hlen) &~ 7;       /* ip databytes per packet */
    if (len < 8)
    {
        error = -1;
        goto bad;
    }

    {
        int mhlen, firstlen = len;
        struct mbuf **mnext = &m->m_nextpkt;
#ifdef VBOX_WITH_SLIRP_BSD_MBUF
        uint8_t buf[len]; /* intermediate buffer we'll use for copy from orriginal packet*/
#endif
            {
                int rc;
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
                rc = LibAliasOut((m->m_la ? m->m_la : pData->proxy_alias), 
                    mtod(m, char *), m->m_len);
#else
                struct m_tag *t;
                if (t = m_tag_find(m, PACKET_TAG_ALIAS, NULL) != 0)
                {
                    rc = LibAliasOut((struct libalias *)&t[1], mtod(m, char *), m_length(m, NULL));
                }
                else
                {
                    rc = LibAliasOut(pData->proxy_alias, mtod(m, char *), 
                                     m_length(m, NULL));
                } 
                if (rc == PKT_ALIAS_IGNORED)
                {
                    Log(("NAT: packet was droppped\n"));
                    goto bad;
                }
#endif
                Log2(("NAT: LibAlias return %d\n", rc));
        }

        /*
         * Loop through length of segment after first fragment,
         * make new header and copy data of each part and link onto chain.
         */
        m0 = m;
        mhlen = sizeof (struct ip);
        for (off = hlen + len; off < (u_int16_t)ip->ip_len; off += len)
        {
            register struct ip *mhip;
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
            m = m_get(pData);
#else
            m = m_gethdr(pData, M_DONTWAIT, MT_HEADER);
#endif
            if (m == 0)
            {
                error = -1;
                ipstat.ips_odropped++;
                goto sendorfree;
            }
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
            m->m_data += if_maxlinkhdr;
            mhip = mtod(m, struct ip *);
            *mhip = *ip;
#else
            m_copyback(pData, m, 0, mhlen, ip);
            m->m_pkthdr.header = mtod(m, void *);
            mhip = mtod(m, struct ip *);
            m_adj(m, mhlen);
#endif
            /* we've calculated eth_dst for first packet */
#if 0 /* No options */
            if (hlen > sizeof (struct ip))
            {
                mhlen = ip_optcopy(ip, mhip) + sizeof (struct ip);
                mhip->ip_hl = mhlen >> 2;
            }
#endif
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
            m->m_len = mhlen;
#endif
            mhip->ip_off = ((off - hlen) >> 3) + (ip->ip_off & ~IP_MF);
            if (ip->ip_off & IP_MF)
                mhip->ip_off |= IP_MF;
            if (off + len >= (u_int16_t)ip->ip_len)
                len = (u_int16_t)ip->ip_len - off;
            else
                mhip->ip_off |= IP_MF;
            mhip->ip_len = htons((u_int16_t)(len + mhlen));

#ifndef VBOX_WITH_SLIRP_BSD_MBUF
            if (m_copy(m, m0, off, len) < 0)
            {
                error = -1;
                goto sendorfree;
            }
#else
            m_copydata(m0, off, len, buf); /* copy to buffer */
            m_append(pData, m, len, buf); /* copy from buffer */
#endif

            mhip->ip_off = htons((u_int16_t)mhip->ip_off);
            mhip->ip_sum = 0;
            mhip->ip_sum = cksum(m, mhlen);
            *mnext = m;
            mnext = &m->m_nextpkt;
            ipstat.ips_ofragments++;
        }
        /*
         * Update first fragment by trimming what's been copied out
         * and updating header, then send each fragment (in order).
         */
        m = m0;
        m_adj(m, hlen + firstlen - (u_int16_t)ip->ip_len);
        ip->ip_len = htons((u_int16_t)m->m_len);
        ip->ip_off = htons((u_int16_t)(ip->ip_off | IP_MF));
        ip->ip_sum = 0;
        ip->ip_sum = cksum(m, hlen);

sendorfree:
        for (m = m0; m; m = m0)
        {
            m0 = m->m_nextpkt;
            m->m_nextpkt = 0;
            if (error == 0)
            {
#ifndef VBOX_WITH_SLIRP_BSD_MBUF
                eh = (struct ethhdr *)MBUF_HEAD(m);
#else
                m = m_prepend(pData, m, ETH_HLEN, M_DONTWAIT);
                Assert(m);
                eh = mtod(m, struct ethhdr *);
#endif
                memcpy(eh->h_source, eth_dst, ETH_ALEN); 

                if_output(pData, so, m);
            }
            else 
            {
                m_freem(pData, m);
            }
        }

        if (error == 0)
            ipstat.ips_fragmented++;
    }

done:
    STAM_PROFILE_STOP(&pData->StatIP_output, a);
    return (error);

bad:
    m_freem(pData, m0);
    STAM_PROFILE_STOP(&pData->StatIP_output, a);
    goto done;
}
