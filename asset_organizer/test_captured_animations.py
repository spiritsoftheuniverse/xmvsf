"""Pure standard-library regression checks for captured MVC pose preview/placement."""
from __future__ import annotations
import csv
import gzip
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from BUILD_CAPTURED_ANIMATIONS import build,group_takes,read_camera,reviewed_records,tile_rgba

class CapturedAnimationsTests(unittest.TestCase):
    def test_lookups_and_original_tile_decode(self):
        b=bytearray(0x500)
        struct.pack_into('<5H',b,0x40,2,1,2,0x36,0x8900)
        for i,p in enumerate(((1,1),(2,1),(3,1))):struct.pack_into('<HH',b,0x4A+i*4,*p)
        review={'items':[{'id':'rom_000040','romOffsetHex':'000040','wanted':True}]}
        r=reviewed_records(b,review)
        self.assertEqual(r['rom_000040']['count'],3)
        self.assertEqual(r['rom_000040']['pairs'],{(1,1),(2,1),(3,1)})
        palette=bytearray(6144)
        struct.pack_into('<H',palette,2*((0x40*16+1)^15),0x0F00)
        gfx=bytes([0x11]*2048)
        pixels=tile_rgba(gfx,len(gfx)-1,0,0,0,palette,2)
        self.assertEqual(pixels[:4],pixels[4:8])
        self.assertEqual(len(pixels),1024)
        self.assertEqual(pixels[3],255)

    def test_takes_split_shot_type_and_sparse_gaps(self):
        items=[];cards={};positions={};camera={}
        frames=[(30,'character'),(90,'character'),(900,'character'),(120,'effect')]
        for i,(frame,tag) in enumerate(frames):
            name=f'rom_{0x40+i*22:06X}'
            items.append({'id':name,'sourceShot':'shot_0001','sourceFrame':frame,'romOffsetHex':name[4:],
                          'tags':[tag],'wanted':True,'componentsMatched':3,'componentsExpected':3})
            cards[name]={'png':'data:image/png;base64,AA==','nativeWidth':12,'nativeHeight':15}
            positions[name]={'screenX':20,'screenY':30,'screenWidth':30,'screenHeight':40}
            camera[frame]={'scroll2x':100,'scroll2y':120,'session':'session_0002'}
        take=group_takes({'items':items},{'groups':[{'id':'candidate_001','wantedAssetIds':[x['id'] for x in items]}]},cards,camera,positions)
        self.assertEqual([x['frameCount'] for x in take],[2,1,1])
        self.assertEqual({x['tag'] for x in take},{'character','effect'})
        self.assertEqual(take[0]['frames'][0]['scroll2Candidate'],{'x':120,'y':150})

    def test_capture_chain_prefers_nonoverlapping_newer_session(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            for name,lo,hi,scroll in [('session_0001',1,5,900),('session_0002',1,3,100),('session_0003',4,6,200)]:
                folder=root/name;folder.mkdir()
                with (folder/'frames.tsv').open('w') as fp:
                    fp.write('frame\tstage\tphase\tscroll1x\tscroll1y\tscroll2x\tscroll2y\tscroll3x\tscroll3y\n')
                    for frame in range(lo,hi+1):fp.write(f'{frame}\t4294967295\t4294967295\t0\t0\t{scroll}\t7\t0\t0\n')
            found,chain=read_camera(root,{2,4,6})
            self.assertEqual(chain,['session_0002','session_0003'])
            self.assertEqual([found[i]['scroll2x'] for i in (2,4,6)],[100,200,200])

    def test_end_to_end_manifest_has_original_screen_bounds_and_no_art_bytes(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);assets=root/'asset_organizer';assets.mkdir()
            trace=root/'private'/'run_1';trace.mkdir(parents=True)
            b=bytearray(0x500)
            for rec in (0x40,0x56):
                struct.pack_into('<5H',b,rec,2,1,2,0x35,0x8200)
                for i,pair in enumerate(((0x10,1),(0x11,2),(0x12,3))):
                    struct.pack_into('<HH',b,rec+10+4*i,*pair)
            (trace/'mvc_program_data_host_words.bin').write_bytes(b)
            digest=hashlib.sha256(b).hexdigest()
            records=[];entries=[]
            for rec,frame in ((0x40,1),(0x56,2)):
                addr=f'{rec:06X}';name=f'rom_{addr}'
                records.append({'id':name,'romOffsetHex':addr,'wanted':True,'sourceShot':'shot_0001',
                    'sourceFrame':frame,'tags':['level_sprite'],'componentsMatched':3,'componentsExpected':3})
                entries.append({'id':name,'offset':addr,'nativeWidth':16,'nativeHeight':16,
                                'png':'data:image/png;base64,AAA='})
            (assets/'aodata.json').write_text(json.dumps({'format':'MVC1_ASSET_ORGANIZER_V2','sourceRomSha256':digest,
                'sourceCatalogSize':2,'items':records}))
            (assets/'wanted_sequences.json').write_text(json.dumps({'sourceRomSha256':digest,'wanted':2,
                'groups':[{'id':'candidate_001','wantedAssetIds':[x['id'] for x in records]}]}))
            (assets/'index.html').write_text('<script type="application/json" id="original-catalog">'+
                json.dumps({'romSha256':digest,'catalog':entries})+'</script>')
            (assets/'CAPTURED_ANIMATIONS_TEMPLATE.html').write_text('<html>__CAPTURED_ANIMATIONS__</html>')
            with gzip.open(root/'sprite_observations.tsv.gz','wt',newline='') as fp:
                w=csv.DictWriter(fp,fieldnames=['shot','frame','tile','attributes','x','y','width_tiles','height_tiles','priority','object_index'],delimiter='\t');w.writeheader()
                for frame in (1,2):
                    for n in range(3):w.writerow({'shot':'shot_0001','frame':frame,'tile':0x10+n,
                        'attributes':n+1,'x':20+16*n,'y':35,'width_tiles':1,'height_tiles':1,'priority':5,'object_index':n})
            capture=root/'capture'/'session_0001';capture.mkdir(parents=True)
            with (capture/'frames.tsv').open('w') as fp:
                fp.write('frame\tstage\tphase\tscroll1x\tscroll1y\tscroll2x\tscroll2y\tscroll3x\tscroll3y\n')
                for frame in (1,2):fp.write(f'{frame}\t4294967295\t4294967295\t0\t0\t80\t50\t0\t0\n')
            meta=build(assets/'aodata.json',assets/'wanted_sequences.json',assets/'index.html',
                root/'sprite_observations.tsv.gz',root/'capture',trace.parent,assets/'CAPTURED_ANIMATIONS_TEMPLATE.html',
                assets/'animations.html',with_backdrops=False)
            self.assertEqual(meta['wanted'],2)
            self.assertEqual(meta['previewTakes'],1)
            self.assertEqual(meta['multiFrameTakes'],1)
            step=meta['takes'][0]['frames'][0]
            self.assertEqual(step['screen']['screenX'],20)
            self.assertEqual(step['scroll2Candidate']['x'],100)
            self.assertNotIn('png',json.dumps(meta))
            self.assertIn('data:image/png;base64', (assets/'animations.html').read_text())

if __name__=='__main__':unittest.main()