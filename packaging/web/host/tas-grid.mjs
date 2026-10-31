// Windowed DOM: a million-sample tape still renders only the visible rows.
export function createTASGrid(root,{call,onSelection,onError,onChange}){
    const names=['↑','↓','←','→','Shift','Space','Q','Esc','Enter'];
    const rowHeight=30,overscan=4;
    let count=0,editable=false,mode='paint',first=0,last=0,anchor=0,drag=null,scheduled=0;
    const table=document.createElement('table');table.className='tas-grid-table';
    const head=table.createTHead(),header=head.insertRow();
    for(const name of ['采样','间隔 ms',...names]){const th=document.createElement('th');th.scope='col';
        if(names.includes(name)){const b=document.createElement('button');b.type='button';b.textContent=name;b.dataset.column=names.indexOf(name);b.title=name==='Enter'?'旧版录像保留位（回放不消费）':'对所选采样填充或清除 '+name;th.append(b)}else th.textContent=name;
        header.append(th);
    }
    const body=table.createTBody();root.replaceChildren(table);
    function select(a,b=a){first=Math.min(a,b);last=Math.max(a,b);onSelection(first,last);render()}
    function spacer(height){if(height<=0)return;const row=body.insertRow(),cell=row.insertCell();row.className='tas-spacer';cell.colSpan=11;cell.style.height=height+'px'}
    function render(){
        if(scheduled){cancelAnimationFrame(scheduled);scheduled=0}
        const scrollTop=root.scrollTop;
        body.replaceChildren();
        for(const button of head.querySelectorAll('button'))button.disabled=!editable||!count;
        if(!count){const row=body.insertRow(),cell=row.insertCell();cell.colSpan=11;cell.textContent='打开录像后显示采样';return}
        const start=Math.max(0,Math.floor((scrollTop-30)/rowHeight)-overscan);
        const end=Math.min(count,start+Math.ceil((root.clientHeight||400)/rowHeight)+overscan*2);
        spacer(start*rowHeight);
        for(let i=start;i<end;++i){
            const row=body.insertRow();row.dataset.sample=i;row.classList.toggle('selected',i>=first&&i<=last);
            const label=document.createElement('button');label.textContent=i;label.dataset.row=i;label.className='sample-number';label.setAttribute('aria-label','选择采样 '+i);row.insertCell().append(label);
            const delta=document.createElement('input');delta.type='number';delta.step='any';delta.min='0.000001';delta.max='1000';delta.value=Number(call('Delta',['number'],[i]).toPrecision(7));delta.dataset.delta=i;delta.disabled=!editable;delta.setAttribute('aria-label','采样 '+i+' 间隔毫秒');row.insertCell().append(delta);
            const keys=call('Keys',['number'],[i]);
            for(let bit=0;bit<9;++bit){const button=document.createElement('button'),down=!!(keys&(1<<bit));button.type='button';button.dataset.row=i;button.dataset.bit=bit;button.disabled=!editable;button.className='key-cell';button.setAttribute('aria-pressed',String(down));button.setAttribute('aria-label','采样 '+i+' '+names[bit]);button.title=names[bit]+(down?'：按下':'：松开');row.insertCell().append(button)}
        }
        spacer((count-end)*rowHeight);
        root.scrollTop=scrollTop;
    }
    function checked(name,types=[],args=[]){if(!call(name,types,args))throw new Error(call('Error',[],[],'string')||'无法修改录像')}
    function finish(){if(drag?.painting)call('EndEdit');drag=null;render()}
    function fail(error){finish();onError(error)}
    function fill(bit){
        if(!editable||!count)return;
        try{
            let all=true;for(let i=first;i<=last&&all;++i)all=!!(call('Keys',['number'],[i])&(1<<bit));
            checked('BeginEdit');checked('SetBitRange',['number','number','number','number'],[first,last,bit,all?0:1]);call('EndEdit');onChange();render();
        }catch(e){call('EndEdit');fail(e)}
    }
    root.addEventListener('scroll',()=>{if(!scheduled)scheduled=requestAnimationFrame(render)});
    root.addEventListener('pointerdown',event=>{
        const cell=event.target.closest('button[data-row]');if(!cell||event.button!==0||!count)return;
        const row=Number(cell.dataset.row),bit=cell.dataset.bit===undefined?null:Number(cell.dataset.bit);
        if(bit!==null&&!editable)return;event.preventDefault();root.focus();
        if(bit===null||mode==='select'){
            if(!event.shiftKey)anchor=row;select(anchor,row);drag={selecting:true};root.setPointerCapture(event.pointerId);return;
        }
        if(mode==='fill'){fill(bit);return}
        try{
            checked('BeginEdit');const down=!(call('Keys',['number'],[row])&(1<<bit));
            checked('SetBitRange',['number','number','number','number'],[row,row,bit,Number(down)]);
            anchor=row;select(row);drag={painting:true,row,bit,down:Number(down)};root.setPointerCapture(event.pointerId);onChange();render();
        }catch(e){call('EndEdit');fail(e)}
    });
    root.addEventListener('pointermove',event=>{
        if(!drag)return;
        const bounds=root.getBoundingClientRect();
        if(event.clientY<bounds.top+38)root.scrollTop-=rowHeight;
        else if(event.clientY>bounds.bottom-24)root.scrollTop+=rowHeight;
        const cell=document.elementFromPoint(event.clientX,event.clientY)?.closest('button[data-row]');if(!cell||!root.contains(cell))return;
        const row=Number(cell.dataset.row);
        if(drag.selecting){select(anchor,row);return}
        if(cell.dataset.bit===undefined)return;
        const bit=Number(cell.dataset.bit),from=bit===drag.bit?Math.min(drag.row,row):row,to=bit===drag.bit?Math.max(drag.row,row):row;
        try{checked('SetBitRange',['number','number','number','number'],[from,to,bit,drag.down]);drag.row=row;drag.bit=bit;select(anchor,row);onChange()}
        catch(e){fail(e)}
    });
    for(const event of ['pointerup','pointercancel','lostpointercapture'])root.addEventListener(event,()=>{if(drag)finish()});
    root.addEventListener('click',event=>{
        const column=event.target.closest('button[data-column]');if(column){fill(Number(column.dataset.column));return}
        const cell=event.target.closest('button[data-row]');if(!cell||event.detail!==0)return;
        const row=Number(cell.dataset.row);if(event.shiftKey)select(anchor,row);else{anchor=row;select(row)}
        if(cell.dataset.bit!==undefined)fill(Number(cell.dataset.bit));
    });
    root.addEventListener('change',event=>{
        const input=event.target;if(input.dataset.delta===undefined)return;
        try{const row=Number(input.dataset.delta),delta=Number(input.value);if(!Number.isFinite(delta)||delta<=0||delta>1000)throw new Error('间隔应大于 0 且不超过 1000 ms');checked('SetRange',['number','number','number','number'],[row,row,call('Keys',['number'],[row]),delta]);onChange();render()}
        catch(e){fail(e)}
    });
    root.addEventListener('keydown',event=>{
        if(event.target!==root||!count)return;
        let row=first;if(event.key==='ArrowDown')row=Math.min(count-1,last+1);else if(event.key==='ArrowUp')row=Math.max(0,first-1);else if(event.key==='Home')row=0;else if(event.key==='End')row=count-1;else return;
        event.preventDefault();if(!event.shiftKey)anchor=row;select(anchor,row);root.scrollTop=Math.max(0,row*rowHeight-root.clientHeight/2);
    });
    new ResizeObserver(()=>render()).observe(root);
    return {load(n){count=n;first=last=anchor=0;root.scrollTop=0;render()},refresh:render,
        setEditable(value){if(editable!==value){editable=value;if(!value)finish();render()}},
        setMode(value){finish();mode=value},
        select(a,b=a,scroll=false){if(!count)return;anchor=a;select(Math.max(0,Math.min(count-1,a)),Math.max(0,Math.min(count-1,b)));if(scroll)root.scrollTop=Math.max(0,first*rowHeight-30)},
        get count(){return count}};
}
