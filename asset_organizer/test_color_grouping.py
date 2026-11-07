"""Offline tests for source-pixel, nonsemantic grouping; no ROM or Pillow."""
from __future__ import annotations
import unittest
from COLOR_GROUPING import palette, lab, similarity, assign_groups

class Image:
    def __init__(self, colors):
        self.pixels=bytearray()
        for rgb,count in colors:
            self.pixels.extend(bytes([*rgb,255])*count)

class ColorTests(unittest.TestCase):
    def test_palette_ignores_transparent_and_preserves_real_rgb(self):
        i=Image([((255,0,0),7),((0,0,255),3)])
        i.pixels.extend(b'\0\xff\0\0'*99)
        result=palette(i)
        self.assertEqual(result[0]['rgb'],[255,0,0]);self.assertEqual(result[0]['pixels'],7)
        self.assertAlmostEqual(result[0]['fraction'],.7)
        self.assertEqual(len(result),2)
    def test_ci_elab_identical_vs_distinct(self):
        a=[(45,33,30,.6),(60,14,11,.4)]
        self.assertAlmostEqual(similarity(a,a),1)
        b=[(70,-25,-30,.6),(80,-15,-10,.4)]
        self.assertLess(similarity(a,b),.5)
        self.assertEqual(tuple(round(x,3) for x in lab((255,255,255)))[0],100.)
    def test_groups_are_from_colors_not_manual_category(self):
        def card(id,offset,rgb,category):
            return {'id':id,'offset':offset,'category':category,
                    'dominantColors':[{'rgb':list(rgb),'fraction':.9,'pixels':100},
                                      {'rgb':[20,30,40],'fraction':.1,'pixels':10}]}
        c=[card('a','000001',(210,25,25),'character'),
           card('b','000002',(210,25,25),'effect'),
           card('c','000003',(10,60,220),'character')]
        result=assign_groups(c,threshold=.8)
        self.assertEqual(result['assetCount'],3)
        self.assertEqual(c[0]['colorGroup'],c[1]['colorGroup'])
        self.assertNotEqual(c[0]['colorGroup'],c[2]['colorGroup'])
        self.assertEqual(len({x['colorGroup'] for x in c}),2)
        self.assertEqual(sum(g['count'] for g in result['groups']),3)


# Source-only validation for the optional manual palette-container field.
# Existing V2 reviews that omit it remain valid and continue to load.
class ReviewColorAssignmentTests(unittest.TestCase):
    def test_palette_override_is_optional_and_whitelisted(self):
        from SERVE_ASSET_ORGANIZER import validate_review
        cat={'romSha256':'a'*64,
             'catalog':[{'id':'rom_000020','offset':'000020'},
                        {'id':'rom_000040','offset':'000040'}],
             'colorAnalysis':{'groups':[{'id':'palette_001'}, {'id':'palette_002'}]}}
        rows=[]
        for off in ('000020','000040'):
            rows.append({'id':'rom_'+off,'romOffsetHex':off,
                         'wanted':False,'reviewStatus':'UNWANTED','tags':['effect']})
        saved={'format':'MVC1_ASSET_ORGANIZER_V2','sourceRomSha256':'a'*64,
               'sourceCatalogSize':2,'items':rows}
        validate_review(saved,cat)
        rows[1]['colorGroupOverride']='palette_002'
        validate_review(saved,cat)
        rows[1]['colorGroupOverride']='palette_199'
        with self.assertRaisesRegex(ValueError,'color assignment'):
            validate_review(saved,cat)
        rows[1]['colorGroupOverride']='../../asset_organizer'
        with self.assertRaisesRegex(ValueError,'color assignment'):
            validate_review(saved,cat)

if __name__=='__main__': unittest.main()