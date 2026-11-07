#!/usr/bin/env python3
"""Group original CPS2 sprite pixels by *measured* palette redundancy.

No guessed character/stage identity is used. Only opaque RGB values from the
locally composited original tiles are examined. Pen zero / transparency is
excluded. CIELAB-near colors receive partial credit; common desaturated/dark
colors are downweighted. A color family is a palette cluster, never a claim
that different sprites are identical art or belong to the same animation.
Dependency-free, deterministic and safe to run only on private local pixels.
"""
from __future__ import annotations
from collections import Counter,defaultdict
import math


def _linear(channel: int) -> float:
    s=channel/255.0
    return s/12.92 if s<=.04045 else ((s+.055)/1.055)**2.4


def lab(rgb: tuple[int,int,int]) -> tuple[float,float,float]:
    r,g,b=(_linear(c) for c in rgb)
    x=(r*.4124564+g*.3575761+b*.1804375)/.95047
    y=r*.2126729+g*.7151522+b*.0721750
    z=(r*.0193339+g*.1191920+b*.9503041)/1.08883
    f=lambda t:t**(1/3) if t>.00885645168 else 7.787037*t+16/116
    a,b,c=f(x),f(y),f(z)
    return 116*b-16,500*(a-b),200*(b-c)


def palette(image,limit:int=16) -> list[dict]:
    """Top RGB colors by opaque pixel frequency, ignoring transparent pixels."""
    if not 1<=limit<=32:raise ValueError('invalid palette limit')
    pixels=image.pixels
    counts=Counter()
    for i in range(0,len(pixels),4):
        if pixels[i+3]>127:counts[(pixels[i],pixels[i+1],pixels[i+2])]+=1
    total=sum(counts.values())
    if total==0:return []
    # Exact original colors are not rounded; same-family detection is tolerant.
    return [{'rgb':list(rgb),'fraction':round(n/total,6),'pixels':n}
            for rgb,n in counts.most_common(limit)]


def _profile(colors:list[dict],frequency:dict,asset_count:int):
    if not colors:return []
    # Common gray/black outlines occur in unrelated artwork. IDF prevents
    # them from spuriously linking everything in a single enormous group.
    entries=[]
    for col in colors:
        rgb=tuple(col['rgb']);l,a,b=lab(rgb)
        rarity=math.log((asset_count+1)/(frequency.get(rgb,0)+1))+1
        chroma=math.hypot(a,b)
        contrast=.62+.38*min(1,chroma/55)
        if l<12:contrast*=.7
        weight=max(1e-6,col['fraction'])**.74*rarity*contrast
        entries.append((l,a,b,weight))
    norm=sum(v[3] for v in entries)
    return [(l,a,b,w/norm) for l,a,b,w in entries]


def similarity(left,right):
    """Symmetric weighted soft palette overlap, in the range 0..1."""
    if not left or not right:return 1.0 if not left and not right else 0.0
    def cover(a,b):
        result=0.
        for l,x,y,w in a:
            nearest=min((l-ll)**2+(x-xx)**2+(y-yy)**2 for ll,xx,yy,_ in b)
            # ∆E76 20 is a visibly different shade: partial, not exact credit.
            result+=w*math.exp(-nearest/(2*19**2))
        return result
    a,b=cover(left,right),cover(right,left)
    return 2*a*b/(a+b) if a+b else 0.


def assign_groups(cards:list[dict],threshold:float=.68) -> dict:
    """Medoid-seeded palette families; every card assigned exactly once.

    Prototype affinity prevents long transitive chains of unrelated art.
    Groups are numbered by size and then stable original ROM offset.
    """
    if not .30<=threshold<=.95:raise ValueError('threshold outside sensible range')
    freq=Counter()
    for card in cards:
        freq.update({tuple(c['rgb']) for c in card.get('dominantColors',[])})
    n=len(cards)
    profiles=[_profile(c.get('dominantColors',[]),freq,n) for c in cards]
    # More chromatically distinct sprites tend to make better family anchors.
    # Resolve ties by stable ROM offset, never by editable user tags/categories.
    order=sorted(range(n),key=lambda i:(-len(profiles[i]),cards[i]['offset']))
    families=[]
    for i in order:
        p=profiles[i];best=(-1.,-1)
        for k,fam in enumerate(families):
            affinity=similarity(p,profiles[fam['representative']])
            if affinity>best[0]:best=(affinity,k)
        if best[0]>=threshold:
            families[best[1]]['members'].append(i)
        else:
            families.append({'representative':i,'members':[i]})
    families.sort(key=lambda f:(-len(f['members']),cards[f['representative']]['offset']))
    groups=[]
    for number,f in enumerate(families,1):
        ident=f'palette_{number:03d}'
        for ix in f['members']:
            cards[ix]['colorGroup']=ident
        pivot=cards[f['representative']]
        swatches=[c['rgb'] for c in pivot.get('dominantColors',[])[:5]]
        groups.append({'id':ident,'label':f'Palette {number:03d}',
                       'count':len(f['members']),'swatches':swatches,
                       'representativeId':pivot['id'],
                       'method':'frequency-weighted CIELAB palette overlap',
                       'threshold':threshold})
    return {'groups':groups,'threshold':threshold,
            'method':'frequency-weighted symmetric CIELAB palette affinity',
            'paletteAccuracy':'RAW_CAPTURE_APPROXIMATE_CPS2_PALETTE',
            'assetCount':n,'groupCount':len(groups)}