#!/usr/bin/env python3
"""Offline all-CPS2-OBJ-priority review tests: no ROM, gameplay or native build."""
import csv
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parent))
import SPRITE_CHANNELS as ch
from EXPORT_CLEAN_RIP import fnv64


class ChannelFactoryTests(unittest.TestCase):
    def make_shot(self,root,number,damage_bg=False):
        shot=root/f'shot_{number:04d}'
        shot.mkdir(parents=True)
        samples=2
        img=bytes((11,44,88,255))*384*224
        with (shot/'layers.tsv').open('w',newline='') as fp:
            writer=csv.writer(fp,delimiter='\t')
            writer.writerow(['sample','emulated_frame','layer','file','fnv64',
                             'width','height','scroll_x','scroll_y'])
            for sample in range(samples):
                for layer in range(4):
                    name=f'l{layer}_{sample:06d}.rgba'
                    (shot/name).write_bytes(img)
                    checksum=fnv64(img) if not (damage_bg and layer==2) else 0
                    writer.writerow([sample,number*1000+sample*30,layer,name,
                                     f'{checksum:016x}',384,224,12*layer,6*layer])
        with (shot/'sprites.tsv').open('w',newline='') as fp:
            writer=csv.writer(fp,delimiter='\t')
            writer.writerow(['sample','object_index','x','y','tile','attributes',
                             'priority','palette','width_tiles','height_tiles'])
            for sample in range(samples):
                for priority in range(8):
                    # SAME logical tile appears on ALL priorities; each must
                    # be indexed independently, not assigned the modal one.
                    writer.writerow([sample,priority,priority*10,25,0x100,
                                     0,priority,0,1,1])
        with (shot/'palettes.bin').open('wb') as fp:
            for sample in range(samples):
                palette=bytearray(2048)
                struct.pack_into('<I',palette,4,0x00aa5500)
                fp.write(struct.pack('<I',sample)+palette)
        (shot/'tiles.bin').write_bytes(struct.pack('<I',0x100)+bytes([0x11])*128)
        (shot/'CAPTURE_COMPLETE.txt').write_text('samples=2\n')
        return shot

    def test_all_eight_priorities_even_with_identical_source_tile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)/'clean_rips'
            root.mkdir()
            self.make_shot(root,1)
            self.make_shot(root,2)
            out=Path(tmp)/'all_channels'
            m=ch.build(root,out,max_previews_per_priority=2,background_shots=2)
            self.assertEqual(len(m['sprite_channels']),8)
            self.assertEqual(m['total_observations'],32)
            self.assertEqual(m['total_indexed_variants'],16)
            self.assertTrue(m['all_eight_priorities_included_even_if_empty'])
            self.assertFalse(m['hud_fighter_effect_scenery_filters_applied'])
            self.assertEqual(len(m['backgrounds']),8)
            for p in range(8):
                c=m['sprite_channels'][p]
                self.assertEqual(c['object_observations'],4)
                self.assertEqual(c['unique_address_variants'],2)
                self.assertEqual(c['review_previews'],2)
                self.assertEqual(len(c['contact_sheets']),1)
                with (out/f'priority_{p}'/'catalog.tsv').open(newline='',encoding='utf-8') as fp:
                    catalog=list(csv.DictReader(fp,delimiter='\t'))
                self.assertEqual(len(catalog),2)
                self.assertTrue(all(int(r['priority'])==p for r in catalog))
                self.assertTrue(all(r['png'].endswith('.png') for r in catalog))
                self.assertTrue((out/c['contact_sheets'][0]).read_bytes().startswith(bytes.fromhex('89504e470d0a1a0a')))
            with zipfile.ZipFile(out/'sprite_all_channels_review_bundle.zip') as z:
                self.assertIsNone(z.testzip())
                self.assertIn('all_priority_observations.tsv.gz',z.namelist())
                self.assertIn('priority_0/catalog.tsv',z.namelist())
                self.assertIn('priority_7/catalog.tsv',z.namelist())
                self.assertIn('scroll_channels/shot_0001_scroll3.png',z.namelist())
            self.assertEqual(m['total_preview_contact_sheets'],8)
            self.assertEqual(ch.build(root,out,max_previews_per_priority=1,
                                      background_shots=1)['total_observations'],32)
            self.assertTrue((root/'shot_0001'/'tiles.bin').exists())

    def test_priority_identity_and_empty_channels_preserved(self):
        obs=[]
        for pri in [0,1,7,7]:
            obs.append({'shot':'shot_0001','sample':0,'frame':100,
                        'index':len(obs),'x':0,'y':0,'tile':0x123,
                        'attributes':0,'priority':pri,'palette':0,
                        'width_tiles':1,'height_tiles':1,'flip':0})
        groups=ch.variants_from_observations(obs)
        self.assertEqual([len(groups[i]) for i in range(8)],[1,1,0,0,0,0,0,1])
        self.assertEqual(groups[7][0]['observations'],2)
        self.assertEqual(groups[7][0]['distinct_frames'],1)

    def test_bad_background_diagnostic_without_overwriting_original(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)/'clean_rips'
            root.mkdir()
            self.make_shot(root,1,damage_bg=True)
            m=ch.build(root,Path(tmp)/'channels',max_previews_per_priority=1,
                       background_shots=1)
            self.assertTrue(any('scroll2' in x and 'FNV64' in x for x in m['warnings']))
            self.assertEqual(len(m['backgrounds']),3)
            with self.assertRaisesRegex(ch.ChannelError,'overlap'):
                ch.build(root,root/'unsafe')
            self.assertFalse((root/'unsafe').exists())

if __name__=='__main__':unittest.main()
