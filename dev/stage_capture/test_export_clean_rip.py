#!/usr/bin/env python3
"""Source-only regression for the existing CPS2 Stage44 clean-layer PNG export."""
import csv
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

spec=importlib.util.spec_from_file_location(
    "clean_importer",Path(__file__).with_name("EXPORT_CLEAN_RIP.py"))
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def unpack_rgba_png(path):
    content=path.read_bytes()
    assert content[:8]==b"\x89PNG\r\n\x1a\n"
    i=8
    w=h=0
    packed=[]
    while i < len(content):
        length=struct.unpack_from(">I",content,i)[0]
        tag=content[i+4:i+8]
        payload=content[i+8:i+8+length]
        assert zlib.crc32(tag+payload)&0xffffffff==struct.unpack_from(">I",content,i+8+length)[0]
        if tag==b"IHDR":
            w,h=struct.unpack_from(">II",payload)
        elif tag==b"IDAT":
            packed.append(payload)
        elif tag==b"IEND":
            break
        i+=length+12
    stream=zlib.decompress(b"".join(packed))
    return w,h,b"".join(stream[y*(w*4+1)+1:(y+1)*(w*4+1)] for y in range(h))


class NativeCleanRipTests(unittest.TestCase):
    def test_transparency_and_native_black_survive(self):
        pic=bytes((0,0,0,255))*384*224
        data=mod.png_bytes(384,224,pic)
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/"native.png"
            path.write_bytes(data)
            w,h,pixels=unpack_rgba_png(path)
            self.assertEqual((w,h),(384,224))
            self.assertEqual(pixels[:8],b"\0\0\0\xff"*2)
            self.assertEqual(pixels,pic)
            blank=mod.png_bytes(384,224,bytes(384*224*4))
            (Path(tmp)/"blank.png").write_bytes(blank)
            self.assertEqual(unpack_rgba_png(Path(tmp)/"blank.png")[2][:4],bytes(4))

    def test_validated_source_and_dedup_per_layer(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            inp=root/"shot_0001"
            inp.mkdir()
            fields=["sample","emulated_frame","layer","file","fnv64",
                    "width","height","scroll_x","scroll_y"]
            rows=[]
            image=bytes((0,0,0,255))*384*224
            second=bytes((22,55,77,255))*384*224
            for sample in (0,1):
                for layer in range(4):
                    raw=second if (sample==1 and layer==2) else image
                    name=f"l{layer}_{sample:06d}.rgba"
                    (inp/name).write_bytes(raw)
                    rows.append([sample,100+2*sample,layer,name,
                                f"{mod.fnv64(raw):016x}",384,224,
                                -1 if layer==0 else 10*layer+sample,
                                -1 if layer==0 else 5*layer+sample])
            with (inp/"layers.tsv").open("w",newline="") as fp:
                writer=csv.writer(fp,delimiter="\t")
                writer.writerow(fields)
                writer.writerows(rows)
            (inp/"CAPTURE_COMPLETE.txt").write_text("samples=2\n")
            target=root/"rendered"
            result=mod.export(inp,target)
            self.assertEqual(result["native_frames"],2)
            self.assertEqual(result["distinct_pngs"],5)
            self.assertEqual(result["duplicates_deduplicated"],3)
            self.assertFalse(result["level_complete"])
            self.assertTrue(result["burst_finished"])
            self.assertEqual(result["frames"][0]["layers"][0]["scroll_x"],-1)
            self.assertEqual(result["frames"][0]["layers"][1]["scroll_x"],10)
            self.assertEqual(result["frames"][1]["layers"][1]["scroll_x"],11)
            self.assertEqual(result["frames"][0]["layers"][1]["path"],
                             result["frames"][1]["layers"][1]["path"])
            self.assertNotEqual(result["frames"][0]["layers"][2]["path"],
                                result["frames"][1]["layers"][2]["path"])
            self.assertEqual(unpack_rgba_png(target/"frames/l0_000000.png")[2][:4],
                             b"\0\0\0\xff")
            with self.assertRaises(FileExistsError):
                mod.export(inp,target)
            with self.assertRaises(ValueError):
                mod.export(inp,inp/"overwrite")

    def test_corrupt_hash_rejected_before_output_creation(self):
        with tempfile.TemporaryDirectory() as tmp:
            inp=Path(tmp)/"shot_0002"
            inp.mkdir()
            with (inp/"layers.tsv").open("w") as fp:
                fp.write("sample\temulated_frame\tlayer\tfile\tfnv64\twidth\theight\tscroll_x\tscroll_y\n")
                fp.write("0\t100\t0\tl0_000000.rgba\t0000000000000000\t384\t224\t-1\t-1\n")
            (inp/"l0_000000.rgba").write_bytes(bytes(384*224*4))
            out=Path(tmp)/"export"
            with self.assertRaisesRegex(ValueError,"FNV64 mismatch"):
                mod.export(inp,out)
            self.assertFalse(out.exists())


if __name__=="__main__":
    unittest.main()
