#!/usr/bin/env python3
"""Offline synthetic CPS2 sprite reconstruction tests; no ROM or game required."""
import csv
import json
import struct
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import SPRITE_FACTORY as sf


class SpriteFactoryTests(unittest.TestCase):
    def test_source_pen_order_palette_and_flip(self):
        pal=bytearray(2048)
        # Native draw 32-bit color is B,G,R,x with pixel nibble index 7.
        struct.pack_into('<I',pal,7*4,0x00aa7744)
        self.assertEqual(sf.effective_pen_color(bytes(pal),0,7),(170,119,68,255))
        tile=bytearray(128)
        tile[:4]=bytes([0x12,0x34,0x56,0x78])
        pens=[(i,i,i,255) if i else (0,0,0,0) for i in range(16)]
        raw=sf.decode_tile(bytes(tile),pens)
        self.assertEqual(raw[:4],bytes([7,7,7,255]))
        self.assertEqual(raw[4:8],bytes([8,8,8,255]))
        self.assertEqual(raw[8*4:9*4],bytes(4))
        flip=sf.decode_tile(bytes(tile),pens,flip_x=True)
        self.assertEqual(flip[15*4:16*4],bytes([7,7,7,255]))
        self.assertEqual(flip[:4],bytes(4))
        yflip=sf.decode_tile(bytes(tile),pens,flip_y=True)
        self.assertEqual(yflip[-16*4:-15*4],bytes([7,7,7,255]))

    def test_multitile_native_nibble_wrapping_and_alignment(self):
        self.assertEqual(sf.sprite_tile_id(0x01F,0,0),0x01F)
        self.assertEqual(sf.sprite_tile_id(0x01F,1,0),0x010)
        self.assertEqual(sf.sprite_tile_id(0x01F,0,1),0x02F)
        pal=bytearray(2048)
        struct.pack_into('<I',pal,1*4,0x00AA5500)
        tile_one=bytes([0x11])*128
        tile_empty=bytes(128)
        tiles={0x01F:tile_one,0x010:tile_empty}
        w,h,pix,missing=sf.compose_sprite(0x01F,0,2,1,0,tiles.get,bytes(pal),True)
        self.assertEqual((w,h,missing),(32,16,[]))
        self.assertEqual(pix[:4],bytes([170,85,0,255]))
        self.assertEqual(pix[16*4:17*4],bytes(4))
        _,_,flip,_=sf.compose_sprite(0x01F,0,2,1,0x20,tiles.get,bytes(pal),True)
        self.assertEqual(flip[:4],bytes(4))
        self.assertEqual(flip[16*4:17*4],bytes([170,85,0,255]))

    def make_shot(self,root:Path,shot='shot_0001',frames=3):
        d=root/shot
        d.mkdir(parents=True)
        with (d/'layers.tsv').open('w',newline='') as fp:
            wr=csv.writer(fp,delimiter='\t')
            wr.writerow(['sample','emulated_frame','layer','file','fnv64',
                         'width','height','scroll_x','scroll_y'])
            for sample in range(frames):
                wr.writerow([sample,100+sample*30,0,f'l0_{sample:06d}.rgba',
                             '00',384,224,-1,-1])
        with (d/'sprites.tsv').open('w',newline='') as fp:
            wr=csv.writer(fp,delimiter='\t')
            wr.writerow(['sample','object_index','x','y','tile','attributes',
                         'priority','palette','width_tiles','height_tiles'])
            for sample in range(frames):
                wr.writerow([sample,9,30+sample,100,0x15,0,3,0,1,1])
        # 3 palette records, one for each sample
        with (d/'palettes.bin').open('wb') as fp:
            for sample in range(frames):
                pal=bytearray(2048)
                struct.pack_into('<I',pal,4,0x00AA5500+sample)
                fp.write(struct.pack('<I',sample)+pal)
        (d/'tiles.bin').write_bytes(struct.pack('<I',0x15)+bytes([0x11])*128)
        return d

    def test_clean_pipeline_creates_exact_tile_png_gallery_and_provenance(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)/'clean_rips'
            root.mkdir()
            self.make_shot(root)
            self.make_shot(root,'shot_0002',frames=2)
            output=Path(td)/'factory'
            m=sf.create_factory(root,None,output)
            self.assertEqual(m['source']['mode'],'effective_native_clean_rip')
            self.assertEqual(m['objects_observed'],5)
            self.assertEqual(m['unique_object_variants'],2)
            self.assertEqual(m['images_exported'],2)
            self.assertTrue(m['source_palette_effective'])
            self.assertFalse(m['scenery_actor_identity_verified'])
            self.assertFalse(m['source_pixels_verified'])
            self.assertEqual(len(m['pages']),1)
            self.assertTrue((output/'sprites'/'shot_0001_T00015_P00_W1_H1_F0.png').is_file())
            with zipfile.ZipFile(output/'sprite_factory_review_bundle.zip') as zipf:
                self.assertEqual(set(zipf.namelist()),{
                    'sprite_factory_manifest.json','sprite_catalog.tsv','sprite_contact_001.png',
                    'sprites/shot_0001_T00015_P00_W1_H1_F0.png',
                    'sprites/shot_0002_T00015_P00_W1_H1_F0.png'})
                self.assertIsNone(zipf.testzip())
            a=(output/'sprite_catalog.tsv').read_text()
            self.assertIn('shot_0002',a)
            self.assertIn('shot_0001',a)
            self.assertTrue((output/'sprite_observations.tsv.gz').is_file())
            self.assertTrue((root/'shot_0001'/'tiles.bin').is_file())
            with self.assertRaisesRegex(sf.SpriteError,'nest'):
                sf.create_factory(root,None,root/'unsafe')
            # Repeated review must not delete source or Stable.
            self.assertEqual(sf.create_factory(root,None,output)['images_exported'],2)

    def test_exact_tile_and_palette_queries_and_missing_source(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)/'clean_rips'
            root.mkdir()
            self.make_shot(root)
            out=Path(td)/'chosen'
            m=sf.create_factory(root,None,out,tile=0x15,palette=0)
            self.assertEqual(m['images_exported'],1)
            out2=Path(td)/'absent'
            m=sf.create_factory(root,None,out2,tile=0x200,palette=0)
            self.assertEqual(m['images_exported'],0)
            self.assertEqual(m['objects_observed'],3)
            with self.assertRaisesRegex(sf.SpriteError,'no clean'):
                sf.create_factory(Path(td)/'missing',None,Path(td)/'missing_output',source='clean')

    def test_missing_tile_never_fabricated(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)/'clean_rips'
            root.mkdir()
            d=self.make_shot(root)
            (d/'tiles.bin').write_bytes(b'')
            m=sf.create_factory(root,None,Path(td)/'factory')
            self.assertEqual(m['images_exported'],0)
            self.assertEqual(m['missing_source_tiles_by_shot']['shot_0001'],1)

if __name__=='__main__':
    unittest.main()