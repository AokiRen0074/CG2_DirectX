from pathlib import Path
import wave,json,hashlib,struct,math
R=Path(__file__).resolve().parents[1];P=R/'Resources/Audio/Selected'
j=json.loads((P/'sources.json').read_text(encoding='utf-8'))
expected={'m01','m05','s03','s08','s11','s14','s15','s21','s22','s23'}
assert {a['id'] for a in j['assets']}==expected
for a in j['assets']:
 p=P/a['file'];assert a['license']=='CC0 1.0' and a['source'].startswith('https://')
 assert hashlib.sha256(p.read_bytes()).hexdigest()==a['sha256']
 with wave.open(str(p),'rb') as w:
  assert w.getsampwidth()==2 and w.getnchannels()==a['channels'] and w.getframerate()==a['sampleRate']
  data=w.readframes(w.getnframes());samples=struct.unpack('<'+'h'*(len(data)//2),data)
  assert max(abs(x) for x in samples)>64 and max(abs(x) for x in samples)<=32767
  if a['id']=='m05':
   ch=w.getnchannels();deltas=[abs(samples[i]-samples[i-ch]) for i in range(ch,len(samples),ch)];deltas.sort()
   seam=max(abs(samples[c]-samples[-ch+c]) for c in range(ch))
   assert seam<=deltas[int(len(deltas)*.999)]*1.5+256, 'Music loop has a discontinuous edge'
assert (P/'CREDITS.txt').exists() and len(list((P/'Licenses').iterdir()))>=5
print('PASS: 10 selected assets, CC0 source records, PCM WAV decode, nonempty audio and continuous battle loop')
