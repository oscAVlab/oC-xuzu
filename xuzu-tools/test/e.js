function euclidean(pulses,steps){
  if(!steps)return[];if(!pulses)return new Array(steps).fill(0);
  if(pulses>=steps)return new Array(steps).fill(1);
  let pattern=[],counts=[],remainders=[],divisor=steps-pulses,level=0;
  remainders.push(pulses);
  while(true){counts.push(Math.floor(divisor/remainders[level]));remainders.push(divisor%remainders[level]);divisor=remainders[level];level++;if(remainders[level]<=1)break;}
  counts.push(divisor);
  function build(l){if(l===-1){pattern.push(0);return;}if(l===-2){pattern.push(1);return;}for(let i=0;i<counts[l];i++)build(l-1);if(remainders[l]!==0)build(l-2);}
  build(level);
  const f=pattern.indexOf(1);if(f>0)pattern=[...pattern.slice(f),...pattern.slice(0,f)];
  return pattern;
}

let out=[];for(let st=1;st<=64;st++)for(let p=0;p<=st;p++){out.push(p+' '+st+' '+euclidean(p,st).join(''));}
console.log(out.join('\n'));