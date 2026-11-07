"""Synthetic regression checks for the offline, non-destructive MVC art review."""
from __future__ import annotations
import csv, gzip, json, struct, tempfile, unittest
from pathlib import Path
import BUILD_MVC_ART_REVIEW as review

class ArtReviewTests(unittest.TestCase):
    def fixture(self,root):
        root=Path(root)
        trace=root/'private'/'run_0001';trace.mkdir(parents=True)
        blob=bytearray(0x400)
        struct.pack_into('<5H',blob,0x40,2,1,2,0x35,0xF801)
        for i,pair in enumerate(((0x10,0x0001),(0x11,0x0011),(0x12,0x0021))):
            struct.pack_into('<HH',blob,0x4A+i*4,*pair)
        (trace/'mvc_program_data_host_words.bin').write_bytes(blob)
        ob=root/'observations.tsv.gz'
        with gzip.open(ob,'wt',newline='') as f:
            names=('shot','frame','tile','attributes','priority','x','y','width_tiles','height_tiles','object_index')
            w=csv.DictWriter(f,fieldnames=names,delimiter='\t');w.writeheader()
            for i,pair in enumerate(((0x10,1),(0x11,0x11),(0x12,0x21))):
                w.writerow(dict(shot='shot_0001',frame=1,tile=pair[0],attributes=pair[1],priority=6,x=16+i*16,y=32,width_tiles=1,height_tiles=1,object_index=i))
        capture=root/'capture'/'session_0001';capture.mkdir(parents=True)
        (capture/'frames.tsv').write_text('frame\tscroll1x\n1\t0\n')
        (capture/'mvc1_decoded_gfx_test.bin').write_bytes(b'\x11'*(8192))
        pal=bytearray(6164);pal[0:4]=b'PAL1';struct.pack_into('<I',pal,4,1)
        struct.pack_into('<H',pal,20+((1^15)*2),0x0fff)
        (capture/'palettes.bin').write_bytes(pal)
        return trace.parent,ob,capture.parent

    def test_tentative_roles_and_source_scoring(self):
        with tempfile.TemporaryDirectory() as t:
            trace,ob,cap=self.fixture(t)
            data,digest=review.get_original(trace)
            ranked,stats=review.rank_records(data,review.read_observations(ob))
            self.assertEqual(len(ranked),1)
            self.assertEqual(ranked[0]['offset'],0x40)
            self.assertEqual(ranked[0]['coverage'],1)
            self.assertEqual(ranked[0]['hits'],3)
            self.assertEqual(len(digest),64)
            self.assertEqual(review.guess_category({'offset':0x327A4E,'coverage':1},review.RGBAImage(30,90))[0],'character')
            self.assertEqual(review.guess_category({'offset':0x7C,'coverage':.75},review.RGBAImage(100,80))[0],'partial')

    def test_original_tile_encoding_and_lossless_png(self):
        import base64,zlib
        colors=(bytes((0,0,0,0)),)+tuple(bytes((i*15,i*8,i*3,255)) for i in range(1,16))
        pixels=bytes((0x11,))*128
        tile=review.decode_original_tile(pixels,colors,False,False)
        self.assertEqual(tile.width,16)
        self.assertEqual(tile.pixels[:4],colors[1])
        png=base64.b64decode(review.encode_png(tile).split(',',1)[1])
        self.assertTrue(png.startswith(b'\x89PNG\r\n\x1a\n'))
        self.assertEqual(struct.unpack_from('>II',png,16),(16,16))
        first_idat=png.find(b'IDAT')
        rowdata=zlib.decompress(png[first_idat+4:first_idat+4+struct.unpack_from('>I',png,first_idat-4)[0]])
        self.assertEqual(rowdata[:5],b'\x00'+colors[1])
        original=review.RGBAImage(16,16)
        original.alpha_composite(tile,(0,0))
        self.assertEqual(original.pixels[:4],colors[1])
        transparent=review.decode_original_tile(bytes(128),colors,False,False)
        original.alpha_composite(transparent,(0,0))
        self.assertEqual(original.pixels[:4],colors[1])
        self.assertEqual(original.getbbox(),(0,0,16,16))
        self.assertEqual(original.crop((0,0,1,1)).pixels[:4],colors[1])

    def test_full_local_html_and_output_safety(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);trace,ob,cap=self.fixture(root)
            template=root/'template.html'
            template.write_text('<html><script type="application/json" id="original-catalog">__MVC_CATALOG__</script></html>')
            out=root/'review.html'
            m=review.create_viewer(trace,ob,cap,template,out)
            self.assertEqual(m['generatedCandidates'],1)
            self.assertEqual(m['fullComponentMatchCards'],1)
            html=out.read_text()
            self.assertIn('data:image/png;base64,',html)
            self.assertIn('rom_000040',html)
            self.assertNotIn('mvc_program_data_host_words.bin',html)
            self.assertTrue(out.with_suffix('.manifest.json').is_file())
            with self.assertRaisesRegex(review.SourceError,'overlaps original'):
                review.create_viewer(trace,ob,cap,template,trace/'review.html')

if __name__=='__main__': unittest.main()
