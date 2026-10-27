(() => {
  // ../../build/pocketjs-upstream/node_modules/solid-js/dist/solid.js
  var sharedConfig = {
    context: undefined,
    registry: undefined,
    effects: undefined,
    done: false,
    getContextId() {
      return getContextId(this.context.count);
    },
    getNextContextId() {
      return getContextId(this.context.count++);
    }
  };
  function getContextId(count) {
    const num = String(count), len = num.length - 1;
    return sharedConfig.context.id + (len ? String.fromCharCode(96 + len) : "") + num;
  }
  function setHydrateContext(context) {
    sharedConfig.context = context;
  }
  function nextHydrateContext() {
    return {
      ...sharedConfig.context,
      id: sharedConfig.getNextContextId(),
      count: 0
    };
  }
  var IS_DEV = false;
  var equalFn = (a, b) => a === b;
  var $PROXY = Symbol("solid-proxy");
  var SUPPORTS_PROXY = typeof Proxy === "function";
  var $TRACK = Symbol("solid-track");
  var $DEVCOMP = Symbol("solid-dev-component");
  var signalOptions = {
    equals: equalFn
  };
  var ERROR = null;
  var runEffects = runQueue;
  var STALE = 1;
  var PENDING = 2;
  var UNOWNED = {
    owned: null,
    cleanups: null,
    context: null,
    owner: null
  };
  var Owner = null;
  var Transition = null;
  var Scheduler = null;
  var ExternalSourceConfig = null;
  var Listener = null;
  var Updates = null;
  var Effects = null;
  var ExecCount = 0;
  function createRoot(fn, detachedOwner) {
    const listener = Listener, owner = Owner, unowned = fn.length === 0, current = detachedOwner === undefined ? owner : detachedOwner, root = unowned ? UNOWNED : {
      owned: null,
      cleanups: null,
      context: current ? current.context : null,
      owner: current
    }, updateFn = unowned ? fn : () => fn(() => untrack(() => cleanNode(root)));
    Owner = root;
    Listener = null;
    try {
      return runUpdates(updateFn, true);
    } finally {
      Listener = listener;
      Owner = owner;
    }
  }
  function createSignal(value, options) {
    options = options ? Object.assign({}, signalOptions, options) : signalOptions;
    const s = {
      value,
      observers: null,
      observerSlots: null,
      comparator: options.equals || undefined
    };
    const setter = (value) => {
      if (typeof value === "function") {
        if (Transition && Transition.running && Transition.sources.has(s))
          value = value(s.tValue);
        else
          value = value(s.value);
      }
      return writeSignal(s, value);
    };
    return [readSignal.bind(s), setter];
  }
  function createRenderEffect(fn, value, options) {
    const c = createComputation(fn, value, false, STALE);
    if (Scheduler && Transition && Transition.running)
      Updates.push(c);
    else
      updateComputation(c);
  }
  function createMemo(fn, value, options) {
    options = options ? Object.assign({}, signalOptions, options) : signalOptions;
    const c = createComputation(fn, value, true, 0);
    c.observers = null;
    c.observerSlots = null;
    c.comparator = options.equals || undefined;
    if (Scheduler && Transition && Transition.running) {
      c.tState = STALE;
      Updates.push(c);
    } else
      updateComputation(c);
    return readSignal.bind(c);
  }
  function batch(fn) {
    return runUpdates(fn, false);
  }
  function untrack(fn) {
    if (!ExternalSourceConfig && Listener === null)
      return fn();
    const listener = Listener;
    Listener = null;
    try {
      if (ExternalSourceConfig)
        return ExternalSourceConfig.untrack(fn);
      return fn();
    } finally {
      Listener = listener;
    }
  }
  function onCleanup(fn) {
    if (Owner === null)
      ;
    else if (Owner.cleanups === null)
      Owner.cleanups = [fn];
    else
      Owner.cleanups.push(fn);
    return fn;
  }
  function startTransition(fn) {
    if (Transition && Transition.running) {
      fn();
      return Transition.done;
    }
    const l = Listener;
    const o = Owner;
    return Promise.resolve().then(() => {
      Listener = l;
      Owner = o;
      let t;
      if (Scheduler || SuspenseContext) {
        t = Transition || (Transition = {
          sources: new Set,
          effects: [],
          promises: new Set,
          disposed: new Set,
          queue: new Set,
          running: true
        });
        t.done || (t.done = new Promise((res) => t.resolve = res));
        t.running = true;
      }
      runUpdates(fn, false);
      Listener = Owner = null;
      return t ? t.done : undefined;
    });
  }
  var [transPending, setTransPending] = /* @__PURE__ */ createSignal(false);
  function createContext(defaultValue, options) {
    const id = Symbol("context");
    return {
      id,
      Provider: createProvider(id),
      defaultValue
    };
  }
  function useContext(context) {
    let value;
    return Owner && Owner.context && (value = Owner.context[context.id]) !== undefined ? value : context.defaultValue;
  }
  function children(fn) {
    const children = createMemo(fn);
    const memo = createMemo(() => resolveChildren(children()));
    memo.toArray = () => {
      const c = memo();
      return Array.isArray(c) ? c : c != null ? [c] : [];
    };
    return memo;
  }
  var SuspenseContext;
  function readSignal() {
    const runningTransition = Transition && Transition.running;
    if (this.sources && (runningTransition ? this.tState : this.state)) {
      if ((runningTransition ? this.tState : this.state) === STALE)
        updateComputation(this);
      else {
        const updates = Updates;
        Updates = null;
        runUpdates(() => lookUpstream(this), false);
        Updates = updates;
      }
    }
    if (Listener) {
      const observers = this.observers;
      if (!observers || observers[observers.length - 1] !== Listener) {
        const sSlot = observers ? observers.length : 0;
        if (!Listener.sources) {
          Listener.sources = [this];
          Listener.sourceSlots = [sSlot];
        } else {
          Listener.sources.push(this);
          Listener.sourceSlots.push(sSlot);
        }
        if (!observers) {
          this.observers = [Listener];
          this.observerSlots = [Listener.sources.length - 1];
        } else {
          observers.push(Listener);
          this.observerSlots.push(Listener.sources.length - 1);
        }
      }
    }
    if (runningTransition && Transition.sources.has(this))
      return this.tValue;
    return this.value;
  }
  function writeSignal(node, value, isComp) {
    let current = Transition && Transition.running && Transition.sources.has(node) ? node.tValue : node.value;
    if (!node.comparator || !node.comparator(current, value)) {
      if (Transition) {
        const TransitionRunning = Transition.running;
        if (TransitionRunning || !isComp && Transition.sources.has(node)) {
          Transition.sources.add(node);
          node.tValue = value;
        }
        if (!TransitionRunning)
          node.value = value;
      } else
        node.value = value;
      if (node.observers && node.observers.length) {
        runUpdates(() => {
          for (let i = 0;i < node.observers.length; i += 1) {
            const o = node.observers[i];
            const TransitionRunning = Transition && Transition.running;
            if (TransitionRunning && Transition.disposed.has(o))
              continue;
            if (TransitionRunning ? !o.tState : !o.state) {
              if (o.pure)
                Updates.push(o);
              else
                Effects.push(o);
              if (o.observers)
                markDownstream(o);
            }
            if (!TransitionRunning)
              o.state = STALE;
            else
              o.tState = STALE;
          }
          if (Updates.length > 1e6) {
            Updates = [];
            if (IS_DEV)
              ;
            throw new Error;
          }
        }, false);
      }
    }
    return value;
  }
  function updateComputation(node) {
    if (!node.fn)
      return;
    cleanNode(node);
    const time = ExecCount;
    runComputation(node, Transition && Transition.running && Transition.sources.has(node) ? node.tValue : node.value, time);
    if (Transition && !Transition.running && Transition.sources.has(node)) {
      queueMicrotask(() => {
        runUpdates(() => {
          Transition && (Transition.running = true);
          Listener = Owner = node;
          runComputation(node, node.tValue, time);
          Listener = Owner = null;
        }, false);
      });
    }
  }
  function runComputation(node, value, time) {
    let nextValue;
    const owner = Owner, listener = Listener;
    Listener = Owner = node;
    try {
      nextValue = node.fn(value);
    } catch (err) {
      if (node.pure) {
        if (Transition && Transition.running) {
          node.tState = STALE;
          node.tOwned && node.tOwned.forEach(cleanNode);
          node.tOwned = undefined;
        } else {
          node.state = STALE;
          node.owned && node.owned.forEach(cleanNode);
          node.owned = null;
        }
      }
      node.updatedAt = time + 1;
      return handleError(err);
    } finally {
      Listener = listener;
      Owner = owner;
    }
    if (!node.updatedAt || node.updatedAt <= time) {
      if (node.updatedAt != null && "observers" in node) {
        writeSignal(node, nextValue, true);
      } else if (Transition && Transition.running && node.pure) {
        if (!Transition.sources.has(node))
          node.value = nextValue;
        Transition.sources.add(node);
        node.tValue = nextValue;
      } else
        node.value = nextValue;
      node.updatedAt = time;
    }
  }
  function createComputation(fn, init, pure, state = STALE, options) {
    const c = {
      fn,
      state,
      updatedAt: null,
      owned: null,
      sources: null,
      sourceSlots: null,
      cleanups: null,
      value: init,
      owner: Owner,
      context: Owner ? Owner.context : null,
      pure
    };
    if (Transition && Transition.running) {
      c.state = 0;
      c.tState = state;
    }
    if (Owner === null)
      ;
    else if (Owner !== UNOWNED) {
      if (Transition && Transition.running && Owner.pure) {
        if (!Owner.tOwned)
          Owner.tOwned = [c];
        else
          Owner.tOwned.push(c);
      } else {
        if (!Owner.owned)
          Owner.owned = [c];
        else
          Owner.owned.push(c);
      }
    }
    if (ExternalSourceConfig && c.fn) {
      const sourceFn = c.fn;
      const [track, trigger] = createSignal(undefined, {
        equals: false
      });
      const ordinary = ExternalSourceConfig.factory(sourceFn, trigger);
      onCleanup(() => ordinary.dispose());
      let inTransition;
      const triggerInTransition = () => startTransition(trigger).then(() => {
        if (inTransition) {
          inTransition.dispose();
          inTransition = undefined;
        }
      });
      c.fn = (x) => {
        track();
        if (Transition && Transition.running) {
          if (!inTransition)
            inTransition = ExternalSourceConfig.factory(sourceFn, triggerInTransition);
          return inTransition.track(x);
        }
        return ordinary.track(x);
      };
    }
    return c;
  }
  function runTop(node) {
    const runningTransition = Transition && Transition.running;
    if ((runningTransition ? node.tState : node.state) === 0)
      return;
    if ((runningTransition ? node.tState : node.state) === PENDING)
      return lookUpstream(node);
    if (node.suspense && untrack(node.suspense.inFallback))
      return node.suspense.effects.push(node);
    const ancestors = [node];
    while ((node = node.owner) && (!node.updatedAt || node.updatedAt < ExecCount)) {
      if (runningTransition && Transition.disposed.has(node))
        return;
      if (runningTransition ? node.tState : node.state)
        ancestors.push(node);
    }
    for (let i = ancestors.length - 1;i >= 0; i--) {
      node = ancestors[i];
      if (runningTransition) {
        let top = node, prev = ancestors[i + 1];
        while ((top = top.owner) && top !== prev) {
          if (Transition.disposed.has(top))
            return;
        }
      }
      if ((runningTransition ? node.tState : node.state) === STALE) {
        updateComputation(node);
      } else if ((runningTransition ? node.tState : node.state) === PENDING) {
        const updates = Updates;
        Updates = null;
        runUpdates(() => lookUpstream(node, ancestors[0]), false);
        Updates = updates;
      }
    }
  }
  function runUpdates(fn, init) {
    if (Updates)
      return fn();
    let wait = false;
    if (!init)
      Updates = [];
    if (Effects)
      wait = true;
    else
      Effects = [];
    ExecCount++;
    try {
      const res = fn();
      completeUpdates(wait);
      return res;
    } catch (err) {
      if (!wait)
        Effects = null;
      Updates = null;
      handleError(err);
    }
  }
  function completeUpdates(wait) {
    if (Updates) {
      if (Scheduler && Transition && Transition.running)
        scheduleQueue(Updates);
      else
        runQueue(Updates);
      Updates = null;
    }
    if (wait)
      return;
    let res;
    if (Transition) {
      if (!Transition.promises.size && !Transition.queue.size) {
        const sources = Transition.sources;
        const disposed = Transition.disposed;
        Effects.push.apply(Effects, Transition.effects);
        res = Transition.resolve;
        for (const e of Effects) {
          "tState" in e && (e.state = e.tState);
          delete e.tState;
        }
        Transition = null;
        runUpdates(() => {
          for (const d of disposed)
            cleanNode(d);
          for (const v of sources) {
            v.value = v.tValue;
            if (v.owned) {
              for (let i = 0, len = v.owned.length;i < len; i++)
                cleanNode(v.owned[i]);
            }
            if (v.tOwned)
              v.owned = v.tOwned;
            delete v.tValue;
            delete v.tOwned;
            v.tState = 0;
          }
          setTransPending(false);
        }, false);
      } else if (Transition.running) {
        Transition.running = false;
        Transition.effects.push.apply(Transition.effects, Effects);
        Effects = null;
        setTransPending(true);
        return;
      }
    }
    const e = Effects;
    Effects = null;
    if (e.length)
      runUpdates(() => runEffects(e), false);
    if (res)
      res();
  }
  function runQueue(queue) {
    for (let i = 0;i < queue.length; i++)
      runTop(queue[i]);
  }
  function scheduleQueue(queue) {
    for (let i = 0;i < queue.length; i++) {
      const item = queue[i];
      const tasks = Transition.queue;
      if (!tasks.has(item)) {
        tasks.add(item);
        Scheduler(() => {
          tasks.delete(item);
          runUpdates(() => {
            Transition.running = true;
            runTop(item);
          }, false);
          Transition && (Transition.running = false);
        });
      }
    }
  }
  function lookUpstream(node, ignore) {
    const runningTransition = Transition && Transition.running;
    if (runningTransition)
      node.tState = 0;
    else
      node.state = 0;
    for (let i = 0;i < node.sources.length; i += 1) {
      const source = node.sources[i];
      if (source.sources) {
        const state = runningTransition ? source.tState : source.state;
        if (state === STALE) {
          if (source !== ignore && (!source.updatedAt || source.updatedAt < ExecCount))
            runTop(source);
        } else if (state === PENDING)
          lookUpstream(source, ignore);
      }
    }
  }
  function markDownstream(node) {
    const runningTransition = Transition && Transition.running;
    for (let i = 0;i < node.observers.length; i += 1) {
      const o = node.observers[i];
      if (runningTransition ? !o.tState : !o.state) {
        if (runningTransition)
          o.tState = PENDING;
        else
          o.state = PENDING;
        if (o.pure)
          Updates.push(o);
        else
          Effects.push(o);
        o.observers && markDownstream(o);
      }
    }
  }
  function cleanNode(node) {
    let i;
    if (node.sources) {
      while (node.sources.length) {
        const source = node.sources.pop(), index = node.sourceSlots.pop(), obs = source.observers;
        if (obs && obs.length) {
          const n = obs.pop(), s = source.observerSlots.pop();
          if (index < obs.length) {
            n.sourceSlots[s] = index;
            obs[index] = n;
            source.observerSlots[index] = s;
          }
        }
      }
    }
    if (node.tOwned) {
      for (i = node.tOwned.length - 1;i >= 0; i--)
        cleanNode(node.tOwned[i]);
      delete node.tOwned;
    }
    if (Transition && Transition.running && node.pure) {
      reset(node, true);
    } else if (node.owned) {
      for (i = node.owned.length - 1;i >= 0; i--)
        cleanNode(node.owned[i]);
      node.owned = null;
    }
    if (node.cleanups) {
      for (i = node.cleanups.length - 1;i >= 0; i--)
        node.cleanups[i]();
      node.cleanups = null;
    }
    if (Transition && Transition.running)
      node.tState = 0;
    else
      node.state = 0;
  }
  function reset(node, top) {
    if (!top) {
      node.tState = 0;
      Transition.disposed.add(node);
    }
    if (node.owned) {
      for (let i = 0;i < node.owned.length; i++)
        reset(node.owned[i]);
    }
  }
  function castError(err) {
    if (err instanceof Error)
      return err;
    return new Error(typeof err === "string" ? err : "Unknown error", {
      cause: err
    });
  }
  function runErrors(err, fns, owner) {
    try {
      for (const f of fns)
        f(err);
    } catch (e) {
      handleError(e, owner && owner.owner || null);
    }
  }
  function handleError(err, owner = Owner) {
    const fns = ERROR && owner && owner.context && owner.context[ERROR];
    const error = castError(err);
    if (!fns)
      throw error;
    if (Effects)
      Effects.push({
        fn() {
          runErrors(error, fns, owner);
        },
        state: STALE
      });
    else
      runErrors(error, fns, owner);
  }
  function resolveChildren(children) {
    if (typeof children === "function" && !children.length)
      return resolveChildren(children());
    if (Array.isArray(children)) {
      const results = [];
      for (let i = 0;i < children.length; i++) {
        const result = resolveChildren(children[i]);
        if (Array.isArray(result)) {
          if (result.length < 32768)
            results.push.apply(results, result);
          else
            for (let j = 0;j < result.length; j++)
              results.push(result[j]);
        } else {
          results.push(result);
        }
      }
      return results;
    }
    return children;
  }
  function createProvider(id, options) {
    return function provider(props) {
      let res;
      createRenderEffect(() => res = untrack(() => {
        Owner.context = {
          ...Owner.context,
          [id]: props.value
        };
        return children(() => props.children);
      }), undefined);
      return res;
    };
  }
  var FALLBACK = Symbol("fallback");
  var hydrationEnabled = false;
  function createComponent(Comp, props) {
    if (hydrationEnabled) {
      if (sharedConfig.context) {
        const c = sharedConfig.context;
        setHydrateContext(nextHydrateContext());
        const r = untrack(() => Comp(props || {}));
        setHydrateContext(c);
        return r;
      }
    }
    return untrack(() => Comp(props || {}));
  }
  function trueFn() {
    return true;
  }
  var propTraps = {
    get(_, property, receiver) {
      if (property === $PROXY)
        return receiver;
      return _.get(property);
    },
    has(_, property) {
      if (property === $PROXY)
        return true;
      return _.has(property);
    },
    set: trueFn,
    deleteProperty: trueFn,
    getOwnPropertyDescriptor(_, property) {
      return {
        configurable: true,
        enumerable: true,
        get() {
          return _.get(property);
        },
        set: trueFn,
        deleteProperty: trueFn
      };
    },
    ownKeys(_) {
      return _.keys();
    }
  };
  function resolveSource(s) {
    return !(s = typeof s === "function" ? s() : s) ? {} : s;
  }
  function resolveSources() {
    for (let i = 0, length = this.length;i < length; ++i) {
      const v = this[i]();
      if (v !== undefined)
        return v;
    }
  }
  function mergeProps(...sources) {
    let proxy = false;
    for (let i = 0;i < sources.length; i++) {
      const s = sources[i];
      proxy = proxy || !!s && $PROXY in s;
      sources[i] = typeof s === "function" ? (proxy = true, createMemo(s)) : s;
    }
    if (SUPPORTS_PROXY && proxy) {
      return new Proxy({
        get(property) {
          for (let i = sources.length - 1;i >= 0; i--) {
            const v = resolveSource(sources[i])[property];
            if (v !== undefined)
              return v;
          }
        },
        has(property) {
          for (let i = sources.length - 1;i >= 0; i--) {
            if (property in resolveSource(sources[i]))
              return true;
          }
          return false;
        },
        keys() {
          const keys = [];
          for (let i = 0;i < sources.length; i++)
            keys.push(...Object.keys(resolveSource(sources[i])));
          return [...new Set(keys)];
        }
      }, propTraps);
    }
    const sourcesMap = {};
    const defined = Object.create(null);
    for (let i = sources.length - 1;i >= 0; i--) {
      const source = sources[i];
      if (!source)
        continue;
      const sourceKeys = Object.getOwnPropertyNames(source);
      for (let i = sourceKeys.length - 1;i >= 0; i--) {
        const key = sourceKeys[i];
        if (key === "__proto__" || key === "constructor")
          continue;
        const desc = Object.getOwnPropertyDescriptor(source, key);
        if (!defined[key]) {
          defined[key] = desc.get ? {
            enumerable: true,
            configurable: true,
            get: resolveSources.bind(sourcesMap[key] = [desc.get.bind(source)])
          } : desc.value !== undefined ? desc : undefined;
        } else {
          const sources = sourcesMap[key];
          if (sources) {
            if (desc.get)
              sources.push(desc.get.bind(source));
            else if (desc.value !== undefined)
              sources.push(() => desc.value);
          }
        }
      }
    }
    const target = {};
    const definedKeys = Object.keys(defined);
    for (let i = definedKeys.length - 1;i >= 0; i--) {
      const key = definedKeys[i], desc = defined[key];
      if (desc && desc.get)
        Object.defineProperty(target, key, desc);
      else
        target[key] = desc ? desc.value : undefined;
    }
    return target;
  }

  // ../../build/pocketjs-upstream/node_modules/solid-js/universal/dist/universal.js
  var memo = (fn) => createMemo(() => fn());
  function createRenderer$1({
    createElement,
    createTextNode,
    isTextNode,
    replaceText,
    insertNode,
    removeNode,
    setProperty,
    getParentNode,
    getFirstChild,
    getNextSibling
  }) {
    function insert(parent, accessor, marker, initial) {
      if (marker !== undefined && !initial)
        initial = [];
      if (typeof accessor !== "function")
        return insertExpression(parent, accessor, initial, marker);
      createRenderEffect((current) => insertExpression(parent, accessor(), current, marker), initial);
    }
    function insertExpression(parent, value, current, marker, unwrapArray) {
      while (typeof current === "function")
        current = current();
      if (value === current)
        return current;
      const t = typeof value, multi = marker !== undefined;
      if (t === "string" || t === "number") {
        if (t === "number")
          value = value.toString();
        if (multi) {
          let node = current[0];
          if (node && isTextNode(node)) {
            replaceText(node, value);
          } else
            node = createTextNode(value);
          current = cleanChildren(parent, current, marker, node);
        } else {
          if (current !== "" && typeof current === "string") {
            replaceText(getFirstChild(parent), current = value);
          } else {
            cleanChildren(parent, current, marker, createTextNode(value));
            current = value;
          }
        }
      } else if (value == null || t === "boolean") {
        current = cleanChildren(parent, current, marker);
      } else if (t === "function") {
        createRenderEffect(() => {
          let v = value();
          while (typeof v === "function")
            v = v();
          current = insertExpression(parent, v, current, marker);
        });
        return () => current;
      } else if (Array.isArray(value)) {
        const array = [];
        if (normalizeIncomingArray(array, value, unwrapArray)) {
          createRenderEffect(() => current = insertExpression(parent, array, current, marker, true));
          return () => current;
        }
        if (array.length === 0) {
          const replacement = cleanChildren(parent, current, marker);
          if (multi)
            return current = replacement;
        } else {
          if (Array.isArray(current)) {
            if (current.length === 0) {
              appendNodes(parent, array, marker);
            } else
              reconcileArrays(parent, current, array);
          } else if (current == null || current === "") {
            appendNodes(parent, array);
          } else {
            reconcileArrays(parent, multi && current || [getFirstChild(parent)], array);
          }
        }
        current = array;
      } else {
        if (Array.isArray(current)) {
          if (multi)
            return current = cleanChildren(parent, current, marker, value);
          cleanChildren(parent, current, null, value);
        } else if (current == null || current === "" || !getFirstChild(parent)) {
          insertNode(parent, value);
        } else
          replaceNode(parent, value, getFirstChild(parent));
        current = value;
      }
      return current;
    }
    function normalizeIncomingArray(normalized, array, unwrap) {
      let dynamic = false;
      for (let i = 0, len = array.length;i < len; i++) {
        let item = array[i], t;
        if (item == null || item === true || item === false)
          ;
        else if (Array.isArray(item)) {
          dynamic = normalizeIncomingArray(normalized, item) || dynamic;
        } else if ((t = typeof item) === "string" || t === "number") {
          normalized.push(createTextNode(item));
        } else if (t === "function") {
          if (unwrap) {
            while (typeof item === "function")
              item = item();
            dynamic = normalizeIncomingArray(normalized, Array.isArray(item) ? item : [item]) || dynamic;
          } else {
            normalized.push(item);
            dynamic = true;
          }
        } else
          normalized.push(item);
      }
      return dynamic;
    }
    function reconcileArrays(parentNode, a, b) {
      let bLength = b.length, aEnd = a.length, bEnd = bLength, aStart = 0, bStart = 0, after = getNextSibling(a[aEnd - 1]), map = null;
      while (aStart < aEnd || bStart < bEnd) {
        if (a[aStart] === b[bStart]) {
          aStart++;
          bStart++;
          continue;
        }
        while (a[aEnd - 1] === b[bEnd - 1]) {
          aEnd--;
          bEnd--;
        }
        if (aEnd === aStart) {
          const node = bEnd < bLength ? bStart ? getNextSibling(b[bStart - 1]) : b[bEnd - bStart] : after;
          while (bStart < bEnd)
            insertNode(parentNode, b[bStart++], node);
        } else if (bEnd === bStart) {
          while (aStart < aEnd) {
            if (!map || !map.has(a[aStart]))
              removeNode(parentNode, a[aStart]);
            aStart++;
          }
        } else if (a[aStart] === b[bEnd - 1] && b[bStart] === a[aEnd - 1]) {
          const node = getNextSibling(a[--aEnd]);
          insertNode(parentNode, b[bStart++], getNextSibling(a[aStart++]));
          insertNode(parentNode, b[--bEnd], node);
          a[aEnd] = b[bEnd];
        } else {
          if (!map) {
            map = new Map;
            let i = bStart;
            while (i < bEnd)
              map.set(b[i], i++);
          }
          const index = map.get(a[aStart]);
          if (index != null) {
            if (bStart < index && index < bEnd) {
              let i = aStart, sequence = 1, t;
              while (++i < aEnd && i < bEnd) {
                if ((t = map.get(a[i])) == null || t !== index + sequence)
                  break;
                sequence++;
              }
              if (sequence > index - bStart) {
                const node = a[aStart];
                while (bStart < index)
                  insertNode(parentNode, b[bStart++], node);
              } else
                replaceNode(parentNode, b[bStart++], a[aStart++]);
            } else
              aStart++;
          } else
            removeNode(parentNode, a[aStart++]);
        }
      }
    }
    function cleanChildren(parent, current, marker, replacement) {
      if (marker === undefined) {
        let removed;
        while (removed = getFirstChild(parent))
          removeNode(parent, removed);
        replacement && insertNode(parent, replacement);
        return "";
      }
      const node = replacement || createTextNode("");
      if (current.length) {
        let inserted = false;
        for (let i = current.length - 1;i >= 0; i--) {
          const el = current[i];
          if (node !== el) {
            const isParent = getParentNode(el) === parent;
            if (!inserted && !i)
              isParent ? replaceNode(parent, node, el) : insertNode(parent, node, marker);
            else
              isParent && removeNode(parent, el);
          } else
            inserted = true;
        }
      } else
        insertNode(parent, node, marker);
      return [node];
    }
    function appendNodes(parent, array, marker) {
      for (let i = 0, len = array.length;i < len; i++)
        insertNode(parent, array[i], marker);
    }
    function replaceNode(parent, newNode, oldNode) {
      insertNode(parent, newNode, oldNode);
      removeNode(parent, oldNode);
    }
    function spreadExpression(node, props, prevProps = {}, skipChildren) {
      props || (props = {});
      if (!skipChildren) {
        createRenderEffect(() => prevProps.children = insertExpression(node, props.children, prevProps.children));
      }
      createRenderEffect(() => props.ref && props.ref(node));
      createRenderEffect(() => {
        for (const prop in props) {
          if (prop === "children" || prop === "ref")
            continue;
          const value = props[prop];
          if (value === prevProps[prop])
            continue;
          setProperty(node, prop, value, prevProps[prop]);
          prevProps[prop] = value;
        }
      });
      return prevProps;
    }
    return {
      render(code, element) {
        let disposer;
        createRoot((dispose) => {
          disposer = dispose;
          insert(element, code());
        });
        return disposer;
      },
      insert,
      spread(node, accessor, skipChildren) {
        if (typeof accessor === "function") {
          createRenderEffect((current) => spreadExpression(node, accessor(), current, skipChildren));
        } else
          spreadExpression(node, accessor, undefined, skipChildren);
      },
      createElement,
      createTextNode,
      insertNode,
      setProp(node, name, value, prev) {
        setProperty(node, name, value, prev);
        return value;
      },
      mergeProps,
      effect: createRenderEffect,
      memo,
      createComponent,
      use(fn, element, arg) {
        return untrack(() => fn(element, arg));
      }
    };
  }
  function createRenderer(options) {
    const renderer = createRenderer$1(options);
    renderer.mergeProps = mergeProps;
    return renderer;
  }

  // ../../build/pocketjs-upstream/framework/src/solid-row.ts
  var RowContext = createContext();
  var nodeRows = new WeakMap;
  function captureNodeRow(node) {
    const row = useContext(RowContext);
    if (row)
      nodeRows.set(node, row);
  }
  function nodeRow(node) {
    return nodeRows.get(node);
  }
  function snapshotValue(value, copies = new Map) {
    if (!value || typeof value !== "object")
      return value;
    if (copies.has(value))
      return copies.get(value);
    if (!Array.isArray(value) && Object.getPrototypeOf(value) !== Object.prototype && Object.getPrototypeOf(value) !== null)
      return value;
    const copy = Array.isArray(value) ? [] : Object.create(Object.getPrototypeOf(value));
    copies.set(value, copy);
    for (const key of Object.keys(value))
      copy[key] = snapshotValue(value[key], copies);
    return copy;
  }
  function withRowSnapshot(row, invoke) {
    if (!row) {
      invoke();
      return;
    }
    const chain = [];
    for (let current = row;current; current = current.parent)
      chain.unshift(current);
    const previous = chain.map((context) => context.snapshot);
    try {
      for (const context of chain) {
        const resolved = context.resolve();
        if (!resolved)
          return;
        context.snapshot = {
          item: snapshotValue(resolved.item),
          index: resolved.index
        };
      }
      invoke();
    } finally {
      for (let i = 0;i < chain.length; i++)
        chain[i].snapshot = previous[i];
    }
  }

  // ../../build/pocketjs-upstream/contracts/spec/binding-generation.ts
  var POCKET_BINDING_GENERATION = 12;

  // ../../build/pocketjs-upstream/contracts/spec/spec.ts
  var SCREEN_W = 480;
  var SCREEN_H = 272;
  var NODE_TYPE = {
    view: 0,
    text: 1,
    image: 2,
    surface: 3
  };
  var ROOT_ID = 1;
  var STYLE_ID_NONE = -1;
  var PROP = {
    width: 1,
    height: 2,
    minW: 3,
    minH: 4,
    maxW: 5,
    maxH: 6,
    paddingT: 8,
    paddingR: 9,
    paddingB: 10,
    paddingL: 11,
    marginT: 12,
    marginR: 13,
    marginB: 14,
    marginL: 15,
    gap: 16,
    flexDir: 17,
    justify: 18,
    align: 19,
    grow: 20,
    shrink: 21,
    basis: 22,
    flexWrap: 23,
    posType: 24,
    insetT: 25,
    insetR: 26,
    insetB: 27,
    insetL: 28,
    display: 29,
    overflow: 30,
    zIndex: 31,
    hitPass: 32,
    contain: 33,
    bgColor: 64,
    gradFrom: 65,
    gradTo: 66,
    gradDir: 67,
    radius: 68,
    opacity: 69,
    borderColor: 70,
    borderWidth: 71,
    shadow: 72,
    bevelOuterLight: 77,
    bevelOuterDark: 78,
    bevelInnerLight: 79,
    bevelInnerDark: 80,
    bevelWidth: 81,
    gradVia: 82,
    gradViaPos: 83,
    textColor: 96,
    fontSlot: 97,
    textAlign: 98,
    lineHeight: 99,
    tracking: 100,
    translateX: 128,
    translateY: 129,
    scale: 130,
    rotate: 131,
    scaleX: 132,
    scaleY: 133,
    originX: 134,
    originY: 135,
    rotateX: 136,
    rotateY: 137,
    translateZ: 138,
    perspective: 139,
    arcStart: 140,
    arcSweep: 141,
    arcWidth: 142,
    skewX: 143
  };
  var ANIMATABLE = [
    "width",
    "height",
    "paddingT",
    "paddingR",
    "paddingB",
    "paddingL",
    "marginT",
    "marginR",
    "marginB",
    "marginL",
    "gap",
    "basis",
    "insetT",
    "insetR",
    "insetB",
    "insetL",
    "bgColor",
    "gradFrom",
    "gradTo",
    "radius",
    "opacity",
    "borderColor",
    "borderWidth",
    "textColor",
    "lineHeight",
    "tracking",
    "translateX",
    "translateY",
    "scale",
    "rotate",
    "scaleX",
    "scaleY",
    "rotateX",
    "rotateY",
    "translateZ",
    "arcStart",
    "arcSweep",
    "arcWidth",
    "skewX"
  ];
  function animBit(prop) {
    return ANIMATABLE.indexOf(prop);
  }
  var VALUE_KIND = {
    f32: 0,
    color: 1,
    int: 2
  };
  var PROP_VALUE_KIND = {
    width: VALUE_KIND.f32,
    height: VALUE_KIND.f32,
    minW: VALUE_KIND.f32,
    minH: VALUE_KIND.f32,
    maxW: VALUE_KIND.f32,
    maxH: VALUE_KIND.f32,
    paddingT: VALUE_KIND.f32,
    paddingR: VALUE_KIND.f32,
    paddingB: VALUE_KIND.f32,
    paddingL: VALUE_KIND.f32,
    marginT: VALUE_KIND.f32,
    marginR: VALUE_KIND.f32,
    marginB: VALUE_KIND.f32,
    marginL: VALUE_KIND.f32,
    gap: VALUE_KIND.f32,
    flexDir: VALUE_KIND.int,
    justify: VALUE_KIND.int,
    align: VALUE_KIND.int,
    grow: VALUE_KIND.f32,
    shrink: VALUE_KIND.f32,
    basis: VALUE_KIND.f32,
    flexWrap: VALUE_KIND.int,
    posType: VALUE_KIND.int,
    insetT: VALUE_KIND.f32,
    insetR: VALUE_KIND.f32,
    insetB: VALUE_KIND.f32,
    insetL: VALUE_KIND.f32,
    display: VALUE_KIND.int,
    overflow: VALUE_KIND.int,
    zIndex: VALUE_KIND.int,
    hitPass: VALUE_KIND.int,
    contain: VALUE_KIND.int,
    bgColor: VALUE_KIND.color,
    gradFrom: VALUE_KIND.color,
    gradTo: VALUE_KIND.color,
    gradDir: VALUE_KIND.int,
    radius: VALUE_KIND.f32,
    opacity: VALUE_KIND.f32,
    borderColor: VALUE_KIND.color,
    borderWidth: VALUE_KIND.f32,
    shadow: VALUE_KIND.int,
    bevelOuterLight: VALUE_KIND.color,
    bevelOuterDark: VALUE_KIND.color,
    bevelInnerLight: VALUE_KIND.color,
    bevelInnerDark: VALUE_KIND.color,
    bevelWidth: VALUE_KIND.f32,
    gradVia: VALUE_KIND.color,
    gradViaPos: VALUE_KIND.f32,
    textColor: VALUE_KIND.color,
    fontSlot: VALUE_KIND.int,
    textAlign: VALUE_KIND.int,
    lineHeight: VALUE_KIND.f32,
    tracking: VALUE_KIND.f32,
    translateX: VALUE_KIND.f32,
    translateY: VALUE_KIND.f32,
    scale: VALUE_KIND.f32,
    rotate: VALUE_KIND.f32,
    scaleX: VALUE_KIND.f32,
    scaleY: VALUE_KIND.f32,
    originX: VALUE_KIND.f32,
    originY: VALUE_KIND.f32,
    rotateX: VALUE_KIND.f32,
    rotateY: VALUE_KIND.f32,
    translateZ: VALUE_KIND.f32,
    perspective: VALUE_KIND.f32,
    arcStart: VALUE_KIND.f32,
    arcSweep: VALUE_KIND.f32,
    arcWidth: VALUE_KIND.f32,
    skewX: VALUE_KIND.f32
  };
  var ENUMS = {
    FlexDir: {
      Row: 0,
      Col: 1
    },
    Justify: {
      Start: 0,
      Center: 1,
      End: 2,
      Between: 3,
      Around: 4
    },
    Align: {
      Start: 0,
      Center: 1,
      End: 2,
      Stretch: 3
    },
    PosType: {
      Relative: 0,
      Absolute: 1
    },
    Display: {
      Flex: 0,
      None: 1
    },
    Overflow: {
      Visible: 0,
      Hidden: 1
    },
    Contain: {
      None: 0,
      Strict: 1,
      Baked: 2
    },
    TextAlign: {
      Left: 0,
      Center: 1,
      Right: 2
    },
    GradDir: {
      ToTop: 0,
      ToBottom: 1,
      ToLeft: 2,
      ToRight: 3
    },
    Easing: {
      Linear: 0,
      EaseIn: 1,
      EaseOut: 2,
      EaseInOut: 3,
      OutBack: 4,
      Spring: 5,
      SpringBouncy: 6,
      CubicBezier: 7
    }
  };
  var PSM = {
    PSM_5650: 0,
    PSM_4444: 2,
    PSM_8888: 3,
    PSM_T8: 5
  };
  var IMG_FLAG_RLE = 1 << 0;
  var IMG_FLAG_LINEAR = 1 << 1;
  var TILESET_FLAG_RLE = 1 << 0;
  var TILESET_FLAG_LINEAR = 1 << 1;
  var SVC_IMG_MAX_BYTES = 128 * 1024;
  var STREAM_FLAG_ENDED = 1 << 0;
  var WIRE_MAX_PAYLOAD = 256 * 1024;
  var WIRE_SLOT_FLAG_RLE = 1 << 0;
  var WIRE_MARK_FLAG_ENDED = 1 << 0;
  var STYLE_VARIANT_BASE = 1 << 0;
  var STYLE_VARIANT_FOCUS = 1 << 1;
  var STYLE_VARIANT_ACTIVE = 1 << 2;
  var STYLE_HAS_TRANSITION = 1 << 3;
  var STYLE_HAS_ANIMATION = 1 << 4;
  var ANIM_FILL_BACKWARDS = 1 << 0;
  var ANIM_FILL_FORWARDS = 1 << 1;
  function abgr(r, g, b, a = 255) {
    return ((a & 255) << 24 | (b & 255) << 16 | (g & 255) << 8 | r & 255) >>> 0;
  }
  var FONT_FLAG_BOLD = 1 << 0;
  var PAK_MAGIC = 1263551300;
  var PAK_VERSION = 1;
  var PAK_HEADER_SIZE = 32;
  var PAK_ENTRY_SIZE = 24;
  var BTN = {
    SELECT: 1,
    START: 8,
    UP: 16,
    RIGHT: 32,
    DOWN: 64,
    LEFT: 128,
    LTRIGGER: 256,
    RTRIGGER: 512,
    ZL: 1024,
    ZR: 2048,
    TRIANGLE: 4096,
    CIRCLE: 8192,
    CROSS: 16384,
    SQUARE: 32768
  };
  var ANALOG_CENTER = 32896;
  var FIXED_DT = 1 / 60;

  // ../../build/pocketjs-upstream/framework/src/host.ts
  function hostViewport(ops) {
    return ops.__viewport ?? null;
  }
  var current = null;
  function embeddedBuildHostContract() {
    const target = "r36s";
    const hostAbi = 12;
    if (hostAbi !== POCKET_BINDING_GENERATION)
      throw new Error(`PocketJS: bundle binding generation ${hostAbi} is unsupported; rebuild for ${POCKET_BINDING_GENERATION}`);
    return target ? {
      target,
      hostAbi
    } : null;
  }
  function assertNativeHostContract(ops, expected = embeddedBuildHostContract()) {
    const baked = 60;
    const declared = ops.__tickHz ?? 60;
    if (declared !== baked) {
      throw new Error(ops.__tickHz === undefined ? `PocketJS: this bundle bakes ${baked} Hz virtual time but the host declares no ui.__tickHz, ` + "which means the 60 Hz default — declare the rate before mount and drive the surface at it " + "(pocket_apple set_tick_rate before eval_bundle; PocketSurfaceView.tickRate)" : `PocketJS: tick-rate mismatch (bundle baked at ${baked} Hz, host drives ${declared} Hz) — ` + "a bundle only runs correctly at the rate it was built with (`--hz`), like glyphs at their density");
    }
    if (expected && expected.hostAbi !== POCKET_BINDING_GENERATION) {
      throw new Error(`PocketJS: native host ABI mismatch (bundle binding generation ${expected.hostAbi} is unsupported; rebuild for ${POCKET_BINDING_GENERATION})`);
    }
    if (ops.__hostAbi !== POCKET_BINDING_GENERATION) {
      throw new Error(`PocketJS: native host ABI mismatch (bundle=${POCKET_BINDING_GENERATION}, host=${ops.__hostAbi ?? "missing"}); rebuild the native host`);
    }
    if (!expected)
      return;
    if (typeof ops.__host !== "string") {
      throw new Error(`PocketJS: this bundle targets "${expected.target}" but the native host predates platform ` + "contracts — add __host/__hostAbi to its ui namespace (see framework/src/host.ts HostOps)");
    }
    if (ops.__host !== expected.target) {
      throw new Error(`PocketJS: native target mismatch (bundle=${expected.target}, host=${ops.__host})`);
    }
  }
  function detectHost(injected) {
    const native = globalThis.ui;
    const nativeMarked = native !== undefined && (typeof native.__host === "string" || native.__textures !== undefined);
    if (injected) {
      if (native !== undefined && injected === native && nativeMarked) {
        assertNativeHostContract(native);
        return {
          ops: injected,
          kind: "native",
          target: native.__host ?? "unknown",
          strict: false
        };
      }
      return {
        ops: injected,
        kind: "injected",
        target: injected.__host ?? "injected",
        strict: true
      };
    }
    if (native !== undefined && nativeMarked) {
      assertNativeHostContract(native);
      return {
        ops: native,
        kind: "native",
        target: native.__host ?? "unknown",
        strict: false
      };
    }
    if (native) {
      return {
        ops: native,
        kind: "injected",
        target: "injected",
        strict: true
      };
    }
    throw new Error("PocketJS: no host — pass HostOps to render() (web/test) or run under a native runtime (globalThis.ui)");
  }
  function installHost(host) {
    current = host;
  }
  function getHost() {
    if (!current) {
      throw new Error("PocketJS: host not installed — call render() first");
    }
    return current;
  }
  function getOps() {
    return getHost().ops;
  }
  function installFrameHandler(fn) {
    globalThis.frame = fn;
  }
  function installResizeViewportHook(resizeViewport) {
    const globals = globalThis;
    const previous = globals.__pocketResizeViewport;
    const hook = (width, height) => resizeViewport(width, height);
    globals.__pocketResizeViewport = hook;
    return () => {
      if (globals.__pocketResizeViewport !== hook)
        return;
      if (previous)
        globals.__pocketResizeViewport = previous;
      else
        delete globals.__pocketResizeViewport;
    };
  }
  function parseHexColor(s) {
    let hex = s.slice(1);
    if (hex.length === 3 || hex.length === 4) {
      hex = [...hex].map((character) => character + character).join("");
    }
    if (hex.length !== 6 && hex.length !== 8) {
      throw new Error(`PocketJS: bad color '${s}' (expected #rgb/#rgba/#rrggbb/#rrggbbaa)`);
    }
    if (!/^[0-9a-fA-F]+$/.test(hex))
      throw new Error(`PocketJS: bad color '${s}'`);
    const n = parseInt(hex, 16);
    if (hex.length === 6) {
      return abgr(n >>> 16 & 255, n >>> 8 & 255, n & 255, 255);
    }
    return abgr(n >>> 24 & 255, n >>> 16 & 255, n >>> 8 & 255, n & 255);
  }
  function encodePropValue(prop, value) {
    const kind = PROP_VALUE_KIND[prop];
    if (typeof value === "string") {
      if (kind === VALUE_KIND.color)
        return parseHexColor(value);
      const n = Number(value);
      if (Number.isNaN(n)) {
        throw new Error(`PocketJS: non-numeric value '${value}' for prop '${prop}'`);
      }
      value = n;
    }
    if (kind === VALUE_KIND.color || kind === VALUE_KIND.int)
      return value >>> 0;
    return value;
  }

  // ../../build/pocketjs-upstream/framework/src/clock.ts
  var TICKS_PER_SECOND = validTickHz(60);
  function validTickHz(hz) {
    if (!Number.isInteger(hz) || hz < 1 || hz > 240) {
      throw new Error(`PocketJS: __POCKET_TICK_HZ__ must be an integer from 1 through 240, got ${hz}`);
    }
    return hz;
  }
  var VALID_HZ = divisorsOf(TICKS_PER_SECOND);
  function divisorsOf(n) {
    const out = [];
    for (let d = 1;d <= n; d++) {
      if (n % d === 0)
        out.push(d);
    }
    return out;
  }
  var hz = TICKS_PER_SECOND;
  var frame = -1;
  var timerSeq = 0;
  var timers = [];
  var nextDue = Infinity;
  function normalizeHz(raw) {
    if (!Number.isFinite(raw) || raw <= 0)
      return TICKS_PER_SECOND;
    let best = VALID_HZ[0];
    for (const v of VALID_HZ) {
      if (Math.abs(v - raw) < Math.abs(best - raw))
        best = v;
    }
    return best;
  }
  function simulationHz() {
    return hz;
  }
  function ticksPerFrame() {
    return TICKS_PER_SECOND / hz;
  }
  function virtualFrame() {
    return frame < 0 ? 0 : frame;
  }
  function virtualNow() {
    return virtualFrame() / hz;
  }
  function resetClock() {
    const raw = globalThis.__simHz;
    hz = typeof raw === "number" ? normalizeHz(raw) : TICKS_PER_SECOND;
    frame = -1;
    timers = [];
    nextDue = Infinity;
    timerSeq = 0;
  }
  function __advanceClock() {
    frame = frame < 0 ? 0 : frame + 1;
    if (timers.length === 0 || frame < nextDue)
      return;
    const due = timers.filter((t) => t.at <= frame).sort((a, b) => a.at - b.at || a.seq - b.seq);
    if (due.length > 0)
      timers = timers.filter((t) => t.at > frame);
    nextDue = Infinity;
    for (const t of timers)
      if (t.at < nextDue)
        nextDue = t.at;
    for (const t of due)
      t.cb();
  }

  // ../../build/pocketjs-upstream/framework/src/analog.ts
  var ANALOG_DEADZONE = 0.12;
  var analogPacked = ANALOG_CENTER;
  var rightPacked = ANALOG_CENTER;
  function __setAnalog(packed, right = undefined) {
    analogPacked = packed === undefined ? ANALOG_CENTER : packed & 65535;
    rightPacked = right === undefined ? ANALOG_CENTER : right & 65535;
  }
  function __resetAnalog() {
    analogPacked = rightPacked = ANALOG_CENTER;
  }
  function axis(raw) {
    const value = Math.max(-1, Math.min(1, (raw - 128) / 127));
    const magnitude = Math.abs(value);
    if (magnitude < ANALOG_DEADZONE)
      return 0;
    return Math.sign(value) * (magnitude - ANALOG_DEADZONE) / (1 - ANALOG_DEADZONE);
  }
  function analogX() {
    return axis(analogPacked >> 8 & 255);
  }
  function analogY() {
    return axis(analogPacked & 255);
  }

  // ../../build/pocketjs-upstream/contracts/spec/motion.ts
  var MotionLevel = {
    Gravity: 1,
    Inertial: 2,
    Geomagnetic: 3
  };
  var MOTION_LEVELS = {
    gravity: MotionLevel.Gravity,
    inertial: MotionLevel.Inertial,
    geomagnetic: MotionLevel.Geomagnetic
  };
  var MotionQuality = {
    Unavailable: 0,
    Unreliable: 1,
    Low: 2,
    Medium: 3,
    High: 4
  };
  var MOTION_MIN_QUALITIES = {
    unreliable: MotionQuality.Unreliable,
    low: MotionQuality.Low,
    medium: MotionQuality.Medium,
    high: MotionQuality.High
  };
  var MotionReferenceFrame = {
    Device: 0,
    Local: 1,
    MagneticNorth: 2,
    TrueNorth: 3
  };
  var MOTION_VALUES = {
    gravityDirection: {
      id: 0,
      level: "gravity",
      components: ["x", "y", "z"],
      metadata: ["quality", "timestamp"]
    },
    inclination: {
      id: 1,
      level: "gravity",
      components: ["degrees"],
      metadata: ["quality", "timestamp"]
    },
    linearAcceleration: {
      id: 2,
      level: "inertial",
      components: ["x", "y", "z"],
      metadata: ["quality", "timestamp"]
    },
    rotationRate: {
      id: 3,
      level: "inertial",
      components: ["x", "y", "z"],
      metadata: ["quality", "timestamp"]
    },
    orientation: {
      id: 4,
      level: "inertial",
      components: ["w", "x", "y", "z"],
      metadata: ["quality", "referenceFrame", "epoch", "timestamp"]
    },
    heading: {
      id: 5,
      level: "geomagnetic",
      components: ["degrees", "accuracy"],
      metadata: ["quality", "referenceFrame", "timestamp"]
    },
    screenRotation: {
      id: 6,
      level: "gravity",
      derivedFrom: "gravityDirection",
      components: ["degrees"],
      metadata: ["quality", "timestamp"]
    },
    tilt: {
      id: 7,
      level: "gravity",
      derivedFrom: "gravityDirection",
      components: ["beta", "gamma"],
      metadata: ["quality", "timestamp"]
    },
    angles: {
      id: 8,
      level: "inertial",
      derivedFrom: "orientation",
      components: ["alpha", "beta", "gamma"],
      metadata: ["quality", "referenceFrame", "epoch", "timestamp"]
    }
  };

  // ../../build/pocketjs-upstream/contracts/spec/microts.ts
  var MICROTS_NUMERIC_TYPES = ["i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "usize", "f32", "f64"];
  var RelativeAxis = {
    Primary: 0,
    Secondary: 1
  };
  var RelativeAxisUnits = {
    PerDegree: 1000,
    PerTurn: 360000
  };
  var MICROTS_RELATIVE_AXES = {
    primary: RelativeAxis.Primary,
    secondary: RelativeAxis.Secondary
  };
  var pixelProps = new Set(["width", "height", "minW", "minH", "maxW", "maxH", "paddingT", "paddingR", "paddingB", "paddingL", "marginT", "marginR", "marginB", "marginL", "gap", "basis", "insetT", "insetR", "insetB", "insetL", "radius", "borderWidth", "bevelWidth", "lineHeight", "tracking", "translateX", "translateY", "translateZ", "perspective", "arcWidth"]);
  var degreeProps = new Set(["rotate", "rotateX", "rotateY", "arcStart", "arcSweep", "skewX"]);
  var MICROTS_ELEMENTS = {
    View: {
      nodeType: NODE_TYPE.view,
      attributes: ["class", "style", "focusable", "debug-name"],
      events: ["press"]
    },
    Text: {
      nodeType: NODE_TYPE.text,
      attributes: ["class"],
      events: []
    },
    Image: {
      nodeType: NODE_TYPE.image,
      attributes: ["class", "src"],
      events: []
    }
  };
  var MICROTS_STYLE_PROPS = Object.fromEntries(Object.entries(PROP).map(([name, id]) => {
    const kind = PROP_VALUE_KIND[name];
    const unit = kind === VALUE_KIND.color ? "Color" : pixelProps.has(name) ? "Px" : degreeProps.has(name) ? "Deg" : undefined;
    return [name, {
      id,
      type: kind === VALUE_KIND.f32 ? "f32" : kind === VALUE_KIND.color ? "u32" : "i32",
      ...unit ? {
        unit
      } : {}
    }];
  }));
  var MICROTS_MATH_BUILTINS = ["sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "exp", "log"];
  var MICROTS_MATH2_BUILTINS = ["atan2", "pow", "hypot"];
  var MICROTS_EXTENDED_BUILTINS = {
    ...Object.fromEntries(MICROTS_NUMERIC_TYPES.map((name) => [name, `export declare function ${name}(value: number): N.${name};`])),
    ...Object.fromEntries(MICROTS_MATH_BUILTINS.map((name) => [name, `export declare function ${name}<T extends N.f32 | N.f64>(value: T): T;`])),
    ...Object.fromEntries(MICROTS_MATH2_BUILTINS.map((name) => [name, `export declare function ${name}<T extends N.f32 | N.f64>(value: T, other: NoInfer<T> | MicroTsPlainNumber): T;`])),
    fill: "export declare function fill<T>(count: N.i32, value: T): T[];",
    push: "export declare function push<T>(target: T[], value: T): void;",
    pop: "export declare function pop<T>(target: T[]): T;",
    insert: "export declare function insert<T>(target: T[], index: N.i32, value: T): void;",
    removeAt: "export declare function removeAt<T>(target: T[], index: N.i32): T;",
    clear: "export declare function clear<T>(target: T[]): void;",
    truncate: "export declare function truncate<T>(target: T[], length: N.i32): void;",
    fillRange: "export declare function fillRange<T>(target: T[], start: N.i32, end: N.i32, value: T): void;",
    copyRange: "export declare function copyRange<T>(target: T[], targetStart: N.i32, source: readonly T[], sourceStart: N.i32, count: N.i32, skip?: T): void;",
    fillRect: "export declare function fillRect<T>(target: T[], start: N.i32, stride: N.i32, width: N.i32, height: N.i32, value: T): void;",
    copyRect: "export declare function copyRect<T>(target: T[], targetStart: N.i32, targetStride: N.i32, source: readonly T[], sourceStart: N.i32, sourceStride: N.i32, width: N.i32, height: N.i32, skip?: T): void;",
    codePoints: "export declare function codePoints(value: string): N.i32[];",
    fromCodePoint: "export declare function fromCodePoint(code: N.i32): string;",
    embedBytes: "export declare function embedBytes(path: string): N.u8[];"
  };

  // ../../build/pocketjs-upstream/framework/src/relative-axis.ts
  var pending = [0, 0];
  var current2 = [0, 0];
  var EMPTY_AXIS_DELTAS = Object.freeze([]);
  function validate(axis, delta) {
    if (axis !== RelativeAxis.Primary && axis !== RelativeAxis.Secondary)
      throw new Error(`Unknown relative axis ${axis}`);
    if (!Number.isInteger(delta) || delta < -2147483648 || delta > 2147483647)
      throw new Error("Relative-axis deltas must be signed i32 millidegrees");
  }
  function sum(a, b) {
    return Math.min(2147483647, Math.max(-2147483648, a + b));
  }
  function __takeAxisDeltas(deltas) {
    if (deltas !== undefined) {
      for (const {
        axis,
        delta
      } of deltas)
        validate(axis, delta);
      pending[0] = pending[1] = 0;
      return deltas;
    }
    const result = pending.some((delta) => delta !== 0) ? pending.flatMap((delta, axis) => delta === 0 ? [] : [{
      axis,
      delta
    }]) : EMPTY_AXIS_DELTAS;
    pending[0] = pending[1] = 0;
    return result;
  }
  function __beginAxisFrame(deltas) {
    current2[0] = current2[1] = 0;
    for (const {
      axis,
      delta
    } of __takeAxisDeltas(deltas)) {
      current2[axis] = sum(current2[axis], delta);
    }
  }
  function __endAxisFrame() {
    current2[0] = current2[1] = 0;
  }
  function __resetAxisInput() {
    pending[0] = pending[1] = current2[0] = current2[1] = 0;
  }

  // ../../build/pocketjs-upstream/framework/src/motion.ts
  var pending2 = null;
  var current3 = null;
  var QUALITIES = Object.values(MotionQuality);
  var FRAMES = Object.values(MotionReferenceFrame);
  function fail(member, rule) {
    throw new Error(`Motion state ${member} must be ${rule}`);
  }
  function f32(value, member) {
    const rounded = typeof value === "number" ? Math.fround(value) + 0 : NaN;
    return Number.isFinite(rounded) ? rounded : fail(member, "a finite f32");
  }
  function quality(value, member) {
    return QUALITIES.includes(value) ? value + 0 : fail(`${member}.quality`, `a MotionQuality id (${QUALITIES.join(", ")})`);
  }
  function referenceFrame(value, member) {
    return FRAMES.includes(value) ? value + 0 : fail(`${member}.referenceFrame`, `a MotionReferenceFrame id (${FRAMES.join(", ")})`);
  }
  function epoch(value, member) {
    return Number.isInteger(value) && value >= 0 && value <= 4294967295 ? value + 0 : fail(`${member}.epoch`, "a u32");
  }
  function components(value, member, length) {
    if (!Array.isArray(value) || value.length !== length)
      fail(`${member}.value`, `${length} numbers`);
    return Array.from(value, (component, index) => f32(component, `${member}.value[${index}]`));
  }
  function estimate(state, member) {
    const value = state[member];
    if (value == null)
      return;
    return typeof value === "object" ? value : fail(member, "an estimate object");
  }
  function __copyMotionState(state) {
    if (state === null || typeof state !== "object")
      throw new Error("A motion state must be an object");
    if (!Number.isSafeInteger(state.timestamp) || state.timestamp < 0)
      fail("timestamp", "non-negative integer microseconds");
    const out = {
      timestamp: state.timestamp + 0
    };
    for (const member of ["gravityDirection", "linearAcceleration", "rotationRate"]) {
      const value = estimate(state, member);
      if (value)
        out[member] = {
          value: components(value.value, member, 3),
          quality: quality(value.quality, member)
        };
    }
    for (const member of ["inclination", "screenRotation"]) {
      const value = estimate(state, member);
      if (value)
        out[member] = {
          value: f32(value.value, `${member}.value`),
          quality: quality(value.quality, member)
        };
    }
    const tilt = estimate(state, "tilt");
    if (tilt)
      out.tilt = {
        value: components(tilt.value, "tilt", 2),
        quality: quality(tilt.quality, "tilt")
      };
    const orientation = estimate(state, "orientation");
    if (orientation) {
      out.orientation = {
        value: components(orientation.value, "orientation", 4),
        quality: quality(orientation.quality, "orientation"),
        referenceFrame: referenceFrame(orientation.referenceFrame, "orientation"),
        epoch: epoch(orientation.epoch, "orientation")
      };
    }
    const angles = estimate(state, "angles");
    if (angles) {
      out.angles = {
        value: components(angles.value, "angles", 3),
        quality: quality(angles.quality, "angles"),
        referenceFrame: referenceFrame(angles.referenceFrame, "angles"),
        epoch: epoch(angles.epoch, "angles")
      };
    }
    const heading = estimate(state, "heading");
    if (heading)
      out.heading = {
        value: f32(heading.value, "heading.value"),
        accuracy: f32(heading.accuracy, "heading.accuracy"),
        quality: quality(heading.quality, "heading"),
        referenceFrame: referenceFrame(heading.referenceFrame, "heading")
      };
    return out;
  }
  function __takeMotionState(state) {
    const result = state === undefined ? pending2 : state === null ? null : __copyMotionState(state);
    pending2 = null;
    return result;
  }
  function __beginMotionFrame(state) {
    current3 = __takeMotionState(state);
  }
  function __endMotionFrame() {
    current3 = null;
  }
  function __resetMotionInput() {
    pending2 = current3 = null;
  }

  // ../../build/pocketjs-upstream/framework/src/frame-dispatch.ts
  class FrameRegistry {
    callbacks = new Set;
    placed = new Map;
    snapshot = null;
    add(callback, placement) {
      if (placement)
        this.placed.set(callback, placement);
      else
        this.callbacks.add(callback);
      this.snapshot = null;
      return () => {
        const removed = this.callbacks.delete(callback);
        if (this.placed.delete(callback) || removed)
          this.snapshot = null;
      };
    }
    clear() {
      this.callbacks.clear();
      this.placed.clear();
      this.snapshot = null;
    }
    freeze() {
      return this.snapshot ??= {
        callbacks: [...this.callbacks],
        placed: [...this.placed]
      };
    }
  }
  function dispatchFrame(frame, buttons, run, beforeHooks, resolveInput) {
    let pending;
    const enqueue = (node, invoke) => {
      pending ??= new Map;
      const entries = pending.get(node);
      if (entries)
        entries.push(invoke);
      else
        pending.set(node, [invoke]);
    };
    beforeHooks?.(enqueue);
    for (let i = 0;i < frame.callbacks.length; i++)
      frame.callbacks[i](buttons);
    for (let i = 0;i < frame.placed.length; i++) {
      const [callback, node] = frame.placed[i];
      enqueue(node, () => callback(buttons));
    }
    resolveInput?.(enqueue);
    if (!pending)
      return;
    const queued = pending;
    const roots = new Set;
    for (const node of queued.keys()) {
      if (!node.isConnected)
        continue;
      let root = node;
      while (root.parent)
        root = root.parent;
      roots.add(root);
    }
    const ordered = [];
    const collect = (node) => {
      const entries = queued.get(node);
      if (entries)
        for (const invoke of entries)
          ordered.push([node, invoke]);
      for (const child of node.children)
        collect(child);
    };
    for (const root of roots)
      collect(root);
    for (const [node, invoke] of ordered)
      run(node, invoke);
  }

  // ../../build/pocketjs-upstream/framework/src/aot-lifecycle.ts
  function createLifecycleScheduler(batch, update = () => {}) {
    const hooks = new Set;
    let sequence = 0;
    let running = false;
    let dirty = false;
    const dead = () => [...hooks].filter((hook) => hook.destroyed).sort((a, b) => b.sequence - a.sequence);
    const destroy = (entries) => {
      for (const hook of entries) {
        hooks.delete(hook);
        hook.unmount?.();
      }
    };
    return {
      register(callback, phase) {
        const hook = {
          sequence: sequence++,
          [phase]: callback,
          destroyed: false,
          mounted: false
        };
        hooks.add(hook);
        dirty = true;
        return () => {
          hook.destroyed = true;
          dirty = true;
        };
      },
      flush() {
        if (running)
          return;
        running = true;
        try {
          update();
          for (let round = 0;; round++) {
            if (!dirty)
              return;
            dirty = false;
            const removed = dead();
            const added = [...hooks].filter((hook) => !hook.destroyed && !hook.mounted).sort((a, b) => a.sequence - b.sequence);
            if (removed.length === 0 && added.length === 0)
              return;
            const ranHooks = removed.some((hook) => hook.unmount) || added.some((hook) => hook.mount);
            if (ranHooks && round >= 8)
              throw new Error("PocketJS: AOT lifecycle exceeded eight hook rounds in one frame");
            batch(() => {
              destroy(removed);
              for (const hook of added) {
                hook.mounted = true;
                hook.mount?.();
              }
            });
            if (!ranHooks)
              return;
            update();
          }
        } catch (error) {
          dirty = true;
          throw error;
        } finally {
          running = false;
        }
      },
      flushUnmounted() {
        batch(() => destroy(dead()));
      },
      reset() {
        hooks.clear();
        sequence = 0;
        running = false;
        dirty = false;
      }
    };
  }

  // ../../build/pocketjs-upstream/framework/src/model-reactive.ts
  var copy = (value) => {
    if (Array.isArray(value))
      return value.map(copy);
    if (value && typeof value === "object")
      return Object.fromEntries(Object.entries(value).map(([k, v]) => [k, copy(v)]));
    return value;
  };
  var regions = new Set;
  function reactModelRegions() {
    for (const region of regions) {
      if (region.disposed) {
        regions.delete(region);
        continue;
      }
      if (!region.initial && region.dirty) {
        region.react();
        region.settle();
      }
    }
  }

  // ../../build/pocketjs-upstream/framework/src/model-tasks.ts
  var services = new Map;
  var deliveries = [];
  function deliverModelResult(request, value) {
    deliveries.push({
      request,
      value
    });
  }
  var key = (r) => `${r.task.region}:${r.task.function}:${r.task.call}:${r.wait}:${r.member}`;
  var taskRuntimes = new Set;
  var lastBoundary;
  var pendingRequests = new Map;
  var commands = [];
  var commandHandler;
  function registerModelCommandHandler(handler) {
    const previous = commandHandler;
    commandHandler = handler;
    return () => {
      if (commandHandler === handler)
        commandHandler = previous;
    };
  }
  function drainModelCommands() {
    const queue = commands;
    commands = [];
    for (const command of queue)
      command();
  }
  function resetModelTaskClock() {
    lastBoundary = undefined;
    for (const runtime of taskRuntimes)
      runtime.resetClock();
  }
  function disposeModelTasks(region) {
    for (const runtime of taskRuntimes)
      if (runtime.region === region)
        runtime.dispose();
  }
  function resumeModelTasks(frame = virtualFrame() + 1, now = virtualNow() * 1000) {
    lastBoundary = {
      frame,
      now
    };
    if (taskRuntimes.size === 0 && deliveries.length === 0)
      return;
    const incoming = deliveries;
    deliveries = [];
    const values = new Map;
    for (const item of incoming) {
      const pending = pendingRequests.get(key(item.request));
      if (pending && pending.task.status === "live" && !pending.runtime.region.disposed) {
        const source = pending.wait.source;
        const value = source.kind === "service" && !services.get(source.service)?.validate(item.value) ? {
          kind: "malformed"
        } : copy(item.value);
        pending.wait.ready = true;
        pending.wait.result = value;
        pending.wait.delivered = true;
        values.set(key(item.request), value);
        pending.runtime.region.emit("delivery", {
          request: item.request,
          value
        });
      } else
        ([...taskRuntimes].find((runtime) => runtime.region.instance === item.request.task.region) ?? taskRuntimes.values().next().value)?.region.emit("delivery-drop", {
          request: item.request
        });
    }
    const ready = [...taskRuntimes].sort((a, b) => a.region.instance - b.region.instance).flatMap((runtime) => runtime.snapshot(frame, now, values));
    for (const run of ready)
      run();
  }

  // ../../build/pocketjs-upstream/framework/src/lifecycle-solid-aot.ts
  var scheduler = createLifecycleScheduler((run) => batch(() => {
    run();
    reactModelRegions();
  }));
  var roots = new WeakSet;
  var models = new Map;
  function registerModelLifecycle(region, root = false) {
    const mount = scheduler.register(() => {
      if (!region.disposed && region.initial) {
        region.react(true);
        region.settle();
      }
    }, "mount");
    const unmount = scheduler.register(() => {
      region.dispose();
      disposeModelTasks(region);
      models.delete(region);
    }, "unmount");
    const dispose = () => {
      mount();
      unmount();
    };
    models.set(region, {
      root,
      dispose
    });
    return dispose;
  }
  function disposeRootModelRegions() {
    for (const entry of models.values())
      if (entry.root)
        entry.dispose();
  }
  function flushLifecycleHooks() {
    scheduler.flush();
    drainModelCommands();
  }
  function flushUnmountedHooks() {
    scheduler.flushUnmounted();
    drainModelCommands();
  }
  function resetLifecycleHooks() {
    scheduler.reset();
    roots = new WeakSet;
    for (const [region, entry] of [...models])
      if (!region.disposed && region.initial)
        registerModelLifecycle(region, entry.root);
  }

  // ../../build/pocketjs-upstream/contracts/spec/platform-types.ts
  function defineCapabilityRegistry(registry) {
    return registry;
  }
  var ANIMATION_CLASSES = ["composited", "repaint", "still"];
  var POCKET_CAPABILITIES = defineCapabilityRegistry([
    "input.analog.left",
    "input.analog.right",
    "input.buttons",
    "input.cursor",
    "input.ime",
    "input.pointer",
    "input.text",
    "input.touch",
    "input.touch.auxiliary",
    "ui.compositor-surfaces",
    "ui.physics",
    "audio.pcm",
    "media.playback",
    "io.offload",
    "net.http",
    "link.companion",
    "system.wifi",
    "camera.qr",
    "camera.photo",
    "system.agent",
    "data.sqlite",
    "data.fs",
    "host.clipboard",
    "display.viewport.live",
    "display.auxiliary",
    "text.glyphs.baked",
    "text.glyphs.streamed",
    "text.glyphs.runtime",
    "text.layout.native",
    "text.layout.offload"
  ]);

  // ../../build/pocketjs-upstream/framework/src/anim.ts
  var animationClass = ANIMATION_CLASSES.includes("composited") ? "composited" : "composited";
  var EASING_BY_NAME = {
    linear: ENUMS.Easing.Linear,
    in: ENUMS.Easing.EaseIn,
    out: ENUMS.Easing.EaseOut,
    "in-out": ENUMS.Easing.EaseInOut,
    "out-back": ENUMS.Easing.OutBack,
    spring: ENUMS.Easing.Spring,
    "spring-bouncy": ENUMS.Easing.SpringBouncy
  };
  function nodeId(node) {
    return typeof node === "number" ? node : node.id;
  }
  function animatablePropId(prop) {
    const propId = PROP[prop];
    if (propId === undefined) {
      throw new Error(`PocketJS: unknown prop '${prop}'`);
    }
    if (animBit(prop) < 0) {
      throw new Error(`PocketJS: prop '${prop}' is not animatable (see spec ANIMATABLE)`);
    }
    return propId;
  }
  function animate(node, prop, to, opts = {}) {
    if (typeof node === "function" && node.__pocketNodeRef)
      throw new Error("NodeSlot animation requires the compiled model transform");
    const propId = animatablePropId(prop);
    if (animationClass === "still") {
      getOps().setProp(nodeId(node), propId, encodePropValue(prop, to));
      return 0;
    }
    let easing;
    if (typeof opts.easing === "number") {
      easing = opts.easing;
    } else {
      const named = EASING_BY_NAME[opts.easing ?? "out"];
      if (named === undefined) {
        throw new Error(`PocketJS: unknown easing '${opts.easing}'`);
      }
      easing = named;
    }
    return getOps().animate(nodeId(node), propId, encodePropValue(prop, to), opts.dur ?? 200, easing, opts.delay ?? 0);
  }
  function jump(node, prop, value) {
    if (typeof node === "function" && node.__pocketNodeRef)
      throw new Error("NodeSlot jump requires the compiled model transform");
    getOps().setProp(nodeId(node), animatablePropId(prop), encodePropValue(prop, value));
  }

  // ../../build/pocketjs-upstream/framework/src/model-node-ref.ts
  var references = new WeakMap;
  function clearNodeReferences(node) {
    for (const child of node.children)
      clearNodeReferences(child);
    for (const slot of references.get(node) ?? [])
      slot.current = null;
    references.delete(node);
  }

  // ../../build/pocketjs-upstream/framework/src/model-animation.ts
  var pending3 = new Map;
  function installModelAnimationCommands() {
    registerModelCommandHandler((region, op, args, request) => {
      if (op === "log") {
        console.log(...args);
        return;
      }
      if (op !== "animate" && op !== "jump")
        throw new Error(`Unknown model command ${op}`);
      const slot = region.refs.get(String(args[0]));
      const node = slot?.current;
      if (!node) {
        if (request)
          deliverModelResult(request, "dropped");
        return;
      }
      if (op === "jump") {
        jump(node, args[1], args[2]);
        return;
      }
      if (request && !getOps().takeAnimationCompletions)
        throw new Error("Host lacks model animation completions; rebuild the host with Model AOT support");
      const id = animate(node, args[1], args[2], args[3]);
      if (request) {
        if (id < 0)
          deliverModelResult(request, "dropped");
        else
          pending3.set(id, request);
        return () => {
          pending3.delete(id);
        };
      }
    });
  }
  installModelAnimationCommands();
  function pollModelAnimations() {
    if (!pending3.size)
      return;
    const raw = getOps().takeAnimationCompletions?.();
    if (!raw)
      return;
    const completions = JSON.parse(raw);
    for (const [id, reason] of completions) {
      const request = pending3.get(id);
      if (!request)
        continue;
      pending3.delete(id);
      deliverModelResult(request, reason === 0 ? "ended" : reason === 1 ? "replaced" : "dropped");
    }
  }

  // ../../build/pocketjs-upstream/framework/src/frame.ts
  var registry = new FrameRegistry;
  var systemRegistry = new FrameRegistry;
  var appSuspended = false;
  var merged = null;
  var NO_FRAME = {
    callbacks: [],
    placed: []
  };
  var buttonHandlerBlockDepth = 0;
  var SystemFrameContext = createContext(false);
  function frameSnapshot() {
    const app = appSuspended ? NO_FRAME : registry.freeze();
    const system = systemRegistry.freeze();
    if (system.callbacks.length === 0 && system.placed.length === 0)
      return app;
    if (merged?.app !== app || merged.system !== system) {
      merged = {
        app,
        system,
        frame: {
          callbacks: [...app.callbacks, ...system.callbacks],
          placed: [...app.placed, ...system.placed]
        }
      };
    }
    return merged.frame;
  }
  var runInRow = (node, invoke) => withRowSnapshot(nodeRow(node), invoke);
  function resetFrameHooks() {
    resetModelTaskClock();
    registry.clear();
    systemRegistry.clear();
    appSuspended = false;
    merged = null;
    buttonHandlerBlockDepth = 0;
    __resetAnalog();
    __resetAxisInput();
    __resetMotionInput();
    resetLifecycleHooks();
  }
  function runFrameHooks(buttons, axisDeltas, resolveInput, beforeHooks, motion) {
    __beginAxisFrame(axisDeltas);
    try {
      __beginMotionFrame(motion);
      const frame = frameSnapshot();
      batch(() => {
        pollModelAnimations();
        resumeModelTasks();
        dispatchFrame(frame, buttons, runInRow, beforeHooks, resolveInput);
        reactModelRegions();
      });
      flushLifecycleHooks();
    } finally {
      __endAxisFrame();
      __endMotionFrame();
    }
  }
  function registerFrame(callback, placement) {
    const row = useContext(RowContext);
    const target = useContext(SystemFrameContext) ? systemRegistry : registry;
    const wrapped = placement ? callback : row ? (buttons) => withRowSnapshot(row, () => callback(buttons)) : (buttons) => callback(buttons);
    const dispose = target.add(wrapped, placement);
    onCleanup(dispose);
    return dispose;
  }
  function onFrame(callback) {
    registerFrame(callback);
  }

  // ../../build/pocketjs-upstream/framework/src/bytes.ts
  var B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  var B64_INV = {};
  for (let i = 0;i < B64.length; i++)
    B64_INV[B64[i]] = i;

  // ../../build/pocketjs-upstream/framework/src/pak.ts
  var map = null;
  var bytes = null;
  function readKey(u8, off, len) {
    let s = "";
    for (let i = 0;i < len; i++)
      s += String.fromCharCode(u8[off + i]);
    return s;
  }
  function parse(ab) {
    const dv = new DataView(ab);
    if (ab.byteLength < PAK_HEADER_SIZE || dv.getUint32(0, true) !== PAK_MAGIC) {
      throw new Error("pak: bad magic");
    }
    const version = dv.getUint16(4, true);
    if (version !== PAK_VERSION) {
      throw new Error("pak: unsupported version " + version);
    }
    const entryCount = dv.getUint32(8, true);
    const dirOff = dv.getUint32(12, true);
    const namesOff = dv.getUint32(16, true);
    const u8 = new Uint8Array(ab);
    const m = new Map;
    for (let i = 0;i < entryCount; i++) {
      const e = dirOff + i * PAK_ENTRY_SIZE;
      const blobOff = dv.getUint32(e + 4, true);
      const byteLen = dv.getUint32(e + 8, true);
      const nameOff = dv.getUint32(e + 12, true);
      const nameLen = dv.getUint16(e + 16, true);
      const dtype = u8[e + 18];
      m.set(readKey(u8, namesOff + nameOff, nameLen), {
        off: blobOff,
        len: byteLen,
        dtype
      });
    }
    map = m;
    bytes = u8;
  }
  function loadPack(ab) {
    parse(ab);
  }
  function ensureLoaded() {
    if (map)
      return;
    const ab = globalThis.__pak;
    if (!ab)
      return;
    parse(ab);
  }
  function hasPack() {
    ensureLoaded();
    return map !== null;
  }
  function entries(prefix = "") {
    ensureLoaded();
    if (!map)
      return [];
    const out = [];
    for (const key of map.keys()) {
      if (key.length >= prefix.length && key.slice(0, prefix.length) === prefix) {
        out.push(key);
      }
    }
    out.sort();
    return out;
  }
  function get(key, start = 0, end) {
    ensureLoaded();
    const e = map ? map.get(key) : undefined;
    if (!e) {
      throw new Error("pak: missing key " + key + " (no __pak provided, or the pack is incomplete)");
    }
    const stop = end ?? e.len;
    if (!Number.isSafeInteger(start) || !Number.isSafeInteger(stop) || start < 0 || stop < start || stop > e.len)
      throw new RangeError("pak: invalid byte range");
    return bytes.slice(e.off + start, e.off + stop);
  }

  // ../../build/pocketjs-upstream/framework/src/input.ts
  var root = null;
  var focused = null;
  var pressedNode = null;
  var prevButtons = 0;
  var deferredPressCapture;
  var focusScopeStack = [];
  var focusGridStack = [];
  var focusControllerStack = [];
  function setInputRoot(r) {
    root = r;
    focused = null;
    pressedNode = null;
    prevButtons = 0;
    focusScopeStack.length = 0;
    focusGridStack.length = 0;
    focusControllerStack.length = 0;
    if (cursor) {
      cursor.pressTarget = null;
      cursor.target = null;
      cursor.spriteDirty = true;
      cursor.fresh = true;
      cursor.vw = 0;
      if (cursor.tex >= 0) {
        const ops = getOps();
        ops.setCursor?.(-1, 0, 0, 0, 0);
        ops.freeTexture?.(cursor.tex);
        cursor.tex = -1;
      }
    }
  }
  function registerPress(node, fn) {
    node.onPress = fn ?? undefined;
  }
  function registerFocusable(node, on) {
    node.focusable = on;
    __notifyTreeMutation();
    if (!on && focused === node) {
      focusNode(null);
    }
  }
  function focusNode(node) {
    if (pressedNode && pressedNode !== node)
      setPressedNode(null);
    focused = node;
    getOps().setFocus(node ? node.id : 0);
  }
  function setPressedNode(node) {
    if (pressedNode === node)
      return;
    const ops = getOps();
    if (pressedNode)
      ops.setActive?.(pressedNode.id, 0);
    pressedNode = node;
    if (node)
      ops.setActive?.(node.id, 1);
  }
  function activeFocusRoot() {
    return focusScopeStack.length > 0 ? focusScopeStack[focusScopeStack.length - 1] : root;
  }
  function collectFocusables(node, out) {
    if (!node)
      return;
    if (node.focusable)
      out.push(node);
    if (!Array.isArray(node.children))
      return;
    for (let i = 0;i < node.children.length; i++) {
      collectFocusables(node.children[i], out);
    }
  }
  function focusables() {
    const out = [];
    const r = activeFocusRoot();
    if (r)
      collectFocusables(r, out);
    return out;
  }
  function linearDirection(direction) {
    return direction === "down" || direction === "right" ? 1 : -1;
  }
  function moveLinearFocus(direction) {
    const dir = linearDirection(direction);
    const list = focusables();
    if (list.length === 0) {
      if (focused)
        focusNode(null);
      return;
    }
    const i = focused ? list.indexOf(focused) : -1;
    if (i < 0) {
      focusNode(dir === 1 ? list[0] : list[list.length - 1]);
      return;
    }
    const j = i + dir;
    if (j < 0 || j >= list.length)
      return;
    focusNode(list[j]);
  }
  function activeGrid() {
    if (!focused)
      return null;
    const active = activeFocusRoot();
    if (active && !isWithin(focused, active))
      return null;
    for (let i = focusGridStack.length - 1;i >= 0; i--) {
      const grid = focusGridStack[i];
      if (active && !isWithin(grid.node, active) && !isWithin(active, grid.node))
        continue;
      if (isWithin(focused, grid.node))
        return grid;
    }
    return null;
  }
  function moveGridFocus(direction) {
    const grid = activeGrid();
    if (!grid)
      return false;
    const list = [];
    collectFocusables(grid.node, list);
    if (list.length === 0) {
      if (focused)
        focusNode(null);
      return true;
    }
    const columns = grid.columns;
    const i = focused ? list.indexOf(focused) : -1;
    if (i < 0) {
      focusNode(linearDirection(direction) === 1 ? list[0] : list[list.length - 1]);
      return true;
    }
    let j = i;
    switch (direction) {
      case "right":
        if (i + 1 < list.length && i % columns < columns - 1)
          j = i + 1;
        else if (grid.wrap)
          j = Math.floor(i / columns) * columns;
        break;
      case "left":
        if (i % columns > 0)
          j = i - 1;
        else if (grid.wrap)
          j = Math.min(list.length - 1, Math.floor(i / columns) * columns + columns - 1);
        break;
      case "down":
        if (i + columns < list.length)
          j = i + columns;
        else if (grid.wrap)
          j = i % columns;
        break;
      case "up":
        if (i - columns >= 0)
          j = i - columns;
        else if (grid.wrap) {
          const col = i % columns;
          j = col;
          while (j + columns < list.length)
            j += columns;
        }
        break;
    }
    if (j !== i)
      focusNode(list[j]);
    return true;
  }
  function activeController() {
    if (!focused)
      return null;
    const active = activeFocusRoot();
    if (active && !isWithin(focused, active))
      return null;
    for (let i = focusControllerStack.length - 1;i >= 0; i--) {
      const ctl = focusControllerStack[i];
      if (active && !isWithin(ctl.node, active) && !isWithin(active, ctl.node))
        continue;
      if (isWithin(focused, ctl.node))
        return ctl;
    }
    return null;
  }
  function moveFocus(direction) {
    const ctl = activeController();
    if (ctl && ctl.move(direction))
      return;
    if (moveGridFocus(direction))
      return;
    moveLinearFocus(direction);
  }
  function withDeferredPress(defer, run) {
    const previous = deferredPressCapture;
    deferredPressCapture = defer;
    try {
      run();
    } finally {
      deferredPressCapture = previous;
    }
  }
  function firePress(defer) {
    firePressFrom(focused, defer);
  }
  function firePressFrom(start, defer = deferredPressCapture) {
    let n = start;
    while (n) {
      if (n.onPress) {
        if (defer) {
          const node = n, callback = n.onPress;
          defer(node, () => callback.call(node));
        } else
          n.onPress();
        return;
      }
      n = n.parent;
    }
  }
  function setActiveNode(node) {
    setPressedNode(node);
  }
  function pressNode(node) {
    focusNode(node);
    firePressFrom(node);
  }
  function isWithin(node, ancestor) {
    if (!node || !ancestor)
      return false;
    let n = node;
    while (n) {
      if (n === ancestor)
        return true;
      n = n.parent;
    }
    return false;
  }
  function firstFocusable(node) {
    if (!node)
      return null;
    if (node.focusable)
      return node;
    if (!Array.isArray(node.children))
      return null;
    for (let i = 0;i < node.children.length; i++) {
      const f = firstFocusable(node.children[i]);
      if (f)
        return f;
    }
    return null;
  }
  function notifyDetached(node) {
    if (!focused || !isWithin(focused, node))
      return;
    const parent = node.parent;
    if (parent) {
      const idx = parent.children.indexOf(node);
      for (let i = idx + 1;i < parent.children.length; i++) {
        const f = firstFocusable(parent.children[i]);
        if (f) {
          focusNode(f);
          return;
        }
      }
      for (let i = idx - 1;i >= 0; i--) {
        const f = firstFocusable(parent.children[i]);
        if (f) {
          focusNode(f);
          return;
        }
      }
      let a = parent;
      while (a) {
        if (a.focusable) {
          focusNode(a);
          return;
        }
        a = a.parent;
      }
    }
    focusNode(null);
  }
  var cursor = null;
  var inputGen = 0;
  function __notifyTreeMutation() {
    inputGen++;
  }
  var ARROW_OUTLINE = [1, 3, 5, 9, 17, 33, 65, 129, 257, 513, 1985, 73, 149, 147, 288, 480];
  var ARROW_FILL = [0, 0, 2, 6, 14, 30, 62, 126, 254, 510, 62, 54, 98, 96, 192, 0];
  function defaultArrowRGBA() {
    const px = new Uint8Array(16 * 16 * 4);
    for (let y = 0;y < 16; y++) {
      for (let x = 0;x < 16; x++) {
        const outline = ARROW_OUTLINE[y] >> x & 1;
        const fill = ARROW_FILL[y] >> x & 1;
        if (!outline && !fill)
          continue;
        const i = (y * 16 + x) * 4;
        const v = fill ? 255 : 0;
        px[i] = v;
        px[i + 1] = v;
        px[i + 2] = v;
        px[i + 3] = 255;
      }
    }
    return px;
  }
  function cursorInitSprite(c, ops) {
    const sprite = c.sprite;
    c.spriteDirty = false;
    const old = c.tex;
    let tex = -1;
    let blob = null;
    if (typeof sprite.image === "string") {
      try {
        blob = get(sprite.image);
      } catch (err) {
        if (getHost().strict)
          throw err;
        blob = null;
      }
    } else if (sprite.image) {
      blob = sprite.image;
    }
    if (blob) {
      tex = ops.uploadImgEntry ? ops.uploadImgEntry(blob) : uploadImgFallback(ops, blob);
      if (tex < 0 && getHost().strict) {
        throw new Error("enableCursor: cursor image rejected (malformed or RLE-only IMG entry)");
      }
    }
    if (tex < 0) {
      tex = ops.uploadTexture(defaultArrowRGBA(), 16, 16, PSM.PSM_8888);
    }
    c.tex = tex;
    ops.setCursor(tex, sprite.hotspot[0], sprite.hotspot[1], sprite.size[0], sprite.size[1]);
    if (old >= 0 && old !== tex)
      ops.freeTexture?.(old);
  }
  function uploadImgFallback(ops, blob) {
    if (blob.length < 8)
      return -1;
    const dv = new DataView(blob.buffer, blob.byteOffset, blob.byteLength);
    if (blob[5] & IMG_FLAG_RLE)
      return -1;
    return ops.uploadTexture(blob.subarray(8), dv.getUint16(0, true), dv.getUint16(2, true), blob[4]);
  }
  function findMirror(node, id) {
    if (!node || id === 0)
      return null;
    if (node.id === id)
      return node;
    const kids = node.children;
    if (!Array.isArray(kids))
      return null;
    for (let i = 0;i < kids.length; i++) {
      const found = findMirror(kids[i], id);
      if (found)
        return found;
    }
    return null;
  }
  var hitRoot = null;
  var auxiliaryHitRoot = null;
  function setHitRoot(r) {
    hitRoot = r;
  }
  function setAuxiliaryHitRoot(r) {
    auxiliaryHitRoot = r;
  }
  function cursorTarget(hit) {
    const scope = focusScopeStack.length > 0 ? focusScopeStack[focusScopeStack.length - 1] : null;
    let n = hit;
    while (n) {
      if (n.focusable && (!scope || isWithin(n, scope)))
        return n;
      n = n.parent;
    }
    return null;
  }
  function touchFocusable(x, y, fact, surface = "primary") {
    return cursorTarget(resolveTouchHit(x, y, fact, surface));
  }
  function resolveTouchHit(x, y, fact, surface = "primary") {
    const searchRoot = surface === "auxiliary" ? auxiliaryHitRoot : hitRoot ?? root;
    if (!searchRoot)
      return null;
    if (fact !== undefined) {
      return fact === 0 ? null : findMirror(searchRoot, fact);
    }
    const ops = getOps();
    const query = surface === "auxiliary" ? ops.hitTestBoundsAuxiliary ?? ops.hitTestAuxiliary : ops.hitTestBounds ?? ops.hitTest;
    if (!query)
      return null;
    return findMirror(searchRoot, query(x, y));
  }
  function cursorFrame(buttons, pressed, released, defer) {
    const c = cursor;
    const ops = getOps();
    if (!ops.hitTest || !ops.setCursor || !ops.setCursorPos)
      return false;
    if (c.vw === 0) {
      const vp = hostViewport(ops);
      c.vw = vp ? vp.w : SCREEN_W;
      c.vh = vp ? vp.h : SCREEN_H;
      if (c.x < 0) {
        c.x = Math.floor(c.vw / 2);
        c.y = Math.floor(c.vh / 2);
      }
    }
    if (c.spriteDirty)
      cursorInitSprite(c, ops);
    let vx = analogX() * c.speed;
    let vy = analogY() * c.speed;
    if (c.dpadSpeed > 0 && vx === 0 && vy === 0) {
      if (buttons & BTN.LEFT)
        vx = -c.dpadSpeed;
      if (buttons & BTN.RIGHT)
        vx = c.dpadSpeed;
      if (buttons & BTN.UP)
        vy = -c.dpadSpeed;
      if (buttons & BTN.DOWN)
        vy = c.dpadSpeed;
    }
    let moved = c.fresh;
    if (vx !== 0 || vy !== 0) {
      const dt = ticksPerFrame() / TICKS_PER_SECOND;
      const nx = Math.min(Math.max(c.x + vx * dt, 0), c.vw - 1);
      const ny = Math.min(Math.max(c.y + vy * dt, 0), c.vh - 1);
      if (nx !== c.x || ny !== c.y) {
        c.x = nx;
        c.y = ny;
        moved = true;
      }
    }
    if (moved)
      ops.setCursorPos(c.x, c.y);
    const edges = (pressed | released) & c.button;
    const gen = inputGen;
    if (moved || edges !== 0 || gen !== c.gen) {
      c.gen = gen;
      c.fresh = false;
      c.target = cursorTarget(findMirror(hitRoot ?? root, ops.hitTest(c.x, c.y)));
    }
    const target = c.target;
    if (target !== focused)
      focusNode(target);
    if (pressed & c.button && target) {
      c.pressTarget = target;
    }
    if (c.pressTarget) {
      setPressedNode(target === c.pressTarget ? c.pressTarget : null);
      if (released & c.button) {
        const fire = target === c.pressTarget;
        c.pressTarget = null;
        setPressedNode(null);
        if (fire)
          firePress(defer);
      }
    } else if (released & c.button) {
      setPressedNode(null);
    }
    return true;
  }
  function handleFrame(buttons, defer) {
    const pressed = buttons & ~prevButtons;
    const released = prevButtons & ~buttons;
    prevButtons = buttons;
    if (cursor && cursorFrame(buttons, pressed, released, defer))
      return;
    if (released & BTN.CIRCLE)
      setPressedNode(null);
    if (pressed === 0)
      return;
    if (pressed & BTN.DOWN)
      moveFocus("down");
    if (pressed & BTN.RIGHT)
      moveFocus("right");
    if (pressed & BTN.UP)
      moveFocus("up");
    if (pressed & BTN.LEFT)
      moveFocus("left");
    if (pressed & BTN.CIRCLE) {
      setPressedNode(focused);
      firePress(defer);
    }
  }

  // ../../build/pocketjs-upstream/framework/src/native-tree.ts
  var treeMutationHook = null;
  function setTreeMutationHook(fn) {
    treeMutationHook = fn;
  }
  function treeMutated() {
    __notifyTreeMutation();
    if (treeMutationHook)
      treeMutationHook();
  }
  function setDebugName(node, name) {
    node.debugName = name || undefined;
    treeMutated();
  }
  var rootMirror = {
    id: ROOT_ID,
    type: NODE_TYPE.view,
    parent: null,
    children: [],
    domNodeType: 1,
    domTag: "root"
  };
  var nativeRoots = new Set([rootMirror]);
  var DOM_NODE = Symbol.for("pocketjs.native-node");
  var DOM_ELEMENT = 1;
  var DOM_TEXT = 3;
  var DOM_COMMENT = 8;
  var NATIVE_ATTRIBUTE_NAMES = new Set(["class", "className", "style", "src", "sprite", "frameStep", "onPress", "on:press", "focusable", "debugName", "ref", "nodeRef", "key", "children"]);
  function domAttrs(node) {
    return node.domAttrs ??= {};
  }
  function cloneNativeNode(node, deep) {
    const nodeType = node.domNodeType ?? (isTextNode(node) ? DOM_TEXT : DOM_ELEMENT);
    const clone = nodeType === DOM_TEXT ? createTextNode(node.text ?? "") : nodeType === DOM_COMMENT ? createCommentNode(node.domData ?? "") : createElement(node.domTag ?? tagName(node));
    for (const key of Object.keys(node.domAttrs ?? {})) {
      setDomAttribute(clone, key, node.domAttrs[key]);
    }
    if (deep) {
      for (const child of node.children)
        insertNode(clone, cloneNativeNode(child, true));
    }
    return clone;
  }
  function setDomAttribute(node, name, value) {
    if (NATIVE_ATTRIBUTE_NAMES.has(name)) {
      setProp(node, name, value, node.domAttrs?.[name]);
      return;
    }
    if (value == null)
      delete domAttrs(node)[name];
    else
      domAttrs(node)[name] = value;
  }
  function decorateNativeNode(node) {
    if (node[DOM_NODE] === true)
      return node;
    Object.defineProperty(node, DOM_NODE, {
      value: true
    });
    Object.defineProperties(node, {
      nodeType: {
        configurable: true,
        get() {
          return node.domNodeType ?? (isTextNode(node) ? DOM_TEXT : DOM_ELEMENT);
        }
      },
      nodeValue: {
        configurable: true,
        get() {
          return node.domNodeType === DOM_COMMENT ? node.domData ?? "" : node.text ?? "";
        },
        set(value) {
          if (node.domNodeType === DOM_COMMENT)
            node.domData = String(value ?? "");
          else
            replaceText(node, String(value ?? ""));
        }
      },
      data: {
        configurable: true,
        get() {
          return node.domNodeType === DOM_COMMENT ? node.domData ?? "" : node.text ?? "";
        },
        set(value) {
          if (node.domNodeType === DOM_COMMENT)
            node.domData = String(value ?? "");
          else
            replaceText(node, String(value ?? ""));
        }
      },
      textContent: {
        configurable: true,
        get() {
          if (node.domNodeType === DOM_COMMENT)
            return node.domData ?? "";
          if (isTextNode(node))
            return node.text ?? "";
          return node.children.map((child) => child.text ?? "").join("");
        },
        set(value) {
          const text = String(value ?? "");
          if (node.domNodeType === DOM_COMMENT) {
            node.domData = text;
          } else if (isTextNode(node)) {
            replaceText(node, text);
          } else {
            clearContainer(node);
            if (text)
              insertNode(node, createTextNode(text));
          }
        }
      },
      parentNode: {
        configurable: true,
        get() {
          return node.parent;
        }
      },
      parentElement: {
        configurable: true,
        get() {
          return node.parent;
        }
      },
      childNodes: {
        configurable: true,
        get() {
          return node.children;
        }
      },
      firstChild: {
        configurable: true,
        get() {
          return node.children[0] ?? null;
        }
      },
      lastChild: {
        configurable: true,
        get() {
          return node.children[node.children.length - 1] ?? null;
        }
      },
      nextSibling: {
        configurable: true,
        get() {
          return getNextSibling(node) ?? null;
        }
      },
      previousSibling: {
        configurable: true,
        get() {
          const parent = node.parent;
          if (!parent)
            return null;
          const index = parent.children.indexOf(node);
          return index > 0 ? parent.children[index - 1] : null;
        }
      },
      tagName: {
        configurable: true,
        get() {
          return (node.domTag ?? tagName(node)).toUpperCase();
        }
      },
      nodeName: {
        configurable: true,
        get() {
          if (node.domNodeType === DOM_TEXT)
            return "#text";
          if (node.domNodeType === DOM_COMMENT)
            return "#comment";
          return (node.domTag ?? tagName(node)).toUpperCase();
        }
      },
      className: {
        configurable: true,
        get() {
          return String(node.domAttrs?.class ?? "");
        },
        set(value) {
          setProp(node, "class", value, node.domAttrs?.class);
        }
      },
      isConnected: {
        configurable: true,
        get() {
          let current = node;
          while (current) {
            if (nativeRoots.has(current))
              return true;
            current = current.parent;
          }
          return false;
        }
      }
    });
    const methods = {
      appendChild(child) {
        insertNode(node, child);
        return child;
      },
      insertBefore(child, anchor) {
        insertNode(node, child, anchor ?? null);
        return child;
      },
      removeChild(child) {
        removeNode(node, child);
        return child;
      },
      replaceChild(next, current) {
        insertNode(node, next, current);
        removeNode(node, current);
        return current;
      },
      cloneNode(deep = false) {
        return cloneNativeNode(node, !!deep);
      },
      remove() {
        if (node.parent)
          removeNode(node.parent, node);
      },
      setAttribute(name, value) {
        setDomAttribute(node, name, value);
      },
      removeAttribute(name) {
        setDomAttribute(node, name, undefined);
      },
      getAttribute(name) {
        const value = node.domAttrs?.[name];
        return value == null ? null : String(value);
      },
      hasAttribute(name) {
        return node.domAttrs?.[name] != null;
      },
      hasChildNodes() {
        return node.children.length > 0;
      },
      contains(other) {
        let current = other ?? null;
        while (current) {
          if (current === node)
            return true;
          current = current.parent;
        }
        return false;
      },
      addEventListener() {},
      removeEventListener() {}
    };
    Object.assign(node, methods, {
      style: {
        length: 0,
        item: () => ""
      },
      classList: {
        add() {},
        remove() {}
      }
    });
    return node;
  }
  decorateNativeNode(rootMirror);
  var styleResolver = null;
  function setStyleResolver(fn) {
    styleResolver = fn;
  }
  var missCounters = {
    unknownClass: 0,
    unknownTexture: 0,
    unknownSurface: 0
  };
  var textures = new Map;
  function registerTexture(key, handle) {
    textures.set(key, handle);
  }
  function resolveTexture(key) {
    const handle = textures.get(key);
    if (handle === undefined) {
      if (getHost().strict) {
        throw new Error(`PocketJS: unknown image src "${key}" - no texture registered under that key`);
      }
      missCounters.unknownTexture++;
    }
    return handle;
  }
  var sprites = new Map;
  function registerSprite(key, meta) {
    sprites.set(key, meta);
  }
  var sweepSet = new Set;
  var retained = new Set;
  function subtreeHasRetained(node) {
    if (!node)
      return false;
    if (retained.has(node))
      return true;
    if (node.children) {
      for (let i = 0;i < node.children.length; i++) {
        if (subtreeHasRetained(node.children[i]))
          return true;
      }
    }
    return false;
  }
  function runSweep() {
    if (sweepSet.size === 0)
      return;
    const ops = getOps();
    const keep = [];
    for (const node of sweepSet) {
      if (!node)
        continue;
      if (node.parent !== null)
        continue;
      if (nativeRoots.has(node))
        continue;
      if (subtreeHasRetained(node)) {
        keep.push(node);
        continue;
      }
      clearNodeReferences(node);
      ops.destroyNode(node.id);
    }
    sweepSet.clear();
    for (let i = 0;i < keep.length; i++)
      sweepSet.add(keep[i]);
  }
  function adoptNativeRoot(id, tag = "native-root") {
    if (!Number.isInteger(id) || id <= ROOT_ID) {
      throw new Error(`PocketJS: invalid adopted native root id ${id}`);
    }
    const root = decorateNativeNode({
      id,
      type: NODE_TYPE.view,
      parent: null,
      children: [],
      domNodeType: DOM_ELEMENT,
      domTag: tag
    });
    nativeRoots.add(root);
    return root;
  }
  function releaseNativeRoot(root) {
    if (root === rootMirror)
      return;
    if (root.children.length > 0) {
      throw new Error("PocketJS: cannot release a native root with mounted children");
    }
    nativeRoots.delete(root);
    sweepSet.delete(root);
    retained.delete(root);
  }
  function createElement(tag) {
    const type = NODE_TYPE[tag];
    if (type === undefined) {
      throw new Error(`PocketJS: unknown element <${tag}> - only view/text/image/surface exist`);
    }
    return decorateNativeNode({
      id: getOps().createNode(type),
      type,
      parent: null,
      children: [],
      domNodeType: DOM_ELEMENT,
      domTag: tag
    });
  }
  function createTextNode(value) {
    value = String(value);
    const ops = getOps();
    const id = ops.createNode(NODE_TYPE.text);
    ops.setText(id, value);
    return decorateNativeNode({
      id,
      type: NODE_TYPE.text,
      parent: null,
      children: [],
      text: value,
      domNodeType: DOM_TEXT,
      domTag: "#text"
    });
  }
  function createCommentNode(data = "") {
    const node = createTextNode("");
    node.domNodeType = DOM_COMMENT;
    node.domTag = "#comment";
    node.domData = data;
    return node;
  }
  function replaceText(node, value) {
    value = String(value);
    getOps().replaceText(node.id, value);
    node.text = value;
    treeMutated();
  }
  function isTextNode(node) {
    return node.type === NODE_TYPE.text;
  }
  function unlink(node) {
    const p = node.parent;
    if (!p)
      return;
    const i = p.children.indexOf(node);
    if (i >= 0)
      p.children.splice(i, 1);
    node.parent = null;
  }
  function insertNode(parent, node, anchor) {
    const ops = getOps();
    if (anchor && (anchor.parent !== parent || !parent.children.includes(anchor))) {
      throw new Error("PocketJS: insert anchor is not a child of parent");
    }
    if (anchor === node) {
      sweepSet.delete(node);
      return;
    }
    unlink(node);
    sweepSet.delete(node);
    ops.insertBefore(parent.id, node.id, anchor ? anchor.id : 0);
    if (anchor) {
      const i = parent.children.indexOf(anchor);
      parent.children.splice(i, 0, node);
    } else {
      parent.children.push(node);
    }
    node.parent = parent;
    treeMutated();
  }
  function removeNode(parent, node) {
    if (!node)
      return;
    notifyDetached(node);
    getOps().removeChild(parent.id, node.id);
    unlink(node);
    sweepSet.add(node);
    treeMutated();
  }
  function getParentNode(node) {
    return node.parent ?? undefined;
  }
  function getFirstChild(node) {
    return node.children[0];
  }
  function getNextSibling(node) {
    const p = node.parent;
    if (!p)
      return;
    const i = p.children.indexOf(node);
    return i >= 0 ? p.children[i + 1] : undefined;
  }
  function setClass(node, value) {
    const ops = getOps();
    treeMutated();
    if (value == null || value === "") {
      ops.setStyle(node.id, STYLE_ID_NONE);
      return;
    }
    if (typeof value !== "string") {
      throw new Error("PocketJS: class must be a string literal of utilities");
    }
    const styleId = styleResolver ? styleResolver(value) : undefined;
    if (styleId === undefined) {
      if (getHost().strict) {
        throw new Error(`PocketJS: unknown class "${value}" - not in the compiled style table ` + "(dynamic classes must be ternaries of full literals)");
      }
      missCounters.unknownClass++;
      return;
    }
    ops.setStyle(node.id, styleId);
  }
  function setSrc(node, value) {
    const ops = getOps();
    if (value == null || value === "") {
      ops.setImage(node.id, -1);
      return;
    }
    if (typeof value !== "string") {
      throw new Error("PocketJS: src must be a string key");
    }
    const handle = resolveTexture(value);
    if (handle !== undefined)
      ops.setImage(node.id, handle);
  }
  function spriteStep(node, fallback, value = node.domAttrs?.frameStep) {
    if (value == null)
      return fallback;
    if (typeof value !== "number" || !Number.isFinite(value)) {
      throw new Error("PocketJS: frameStep must be a finite number");
    }
    return Math.min(65535, Math.max(1, Math.floor(value)));
  }
  function setSpriteSrc(node, value) {
    const ops = getOps();
    if (value == null || value === "") {
      ops.setSprite(node.id, -1, 0, 0, 0);
      return;
    }
    if (typeof value !== "string") {
      throw new Error("PocketJS: sprite must be a string key");
    }
    const meta = sprites.get(value);
    if (meta === undefined) {
      if (getHost().strict) {
        throw new Error(`PocketJS: unknown sprite "${value}" - no sprite atlas registered under that key`);
      }
      missCounters.unknownTexture++;
      return;
    }
    ops.setSprite(node.id, meta.handle, meta.frames, meta.cols, spriteStep(node, meta.step));
  }
  function setCompositorBinding(node) {
    const ops = getOps();
    const bind = ops.setCompositorSurface;
    if (!bind) {
      return;
    }
    if (!ops.__surfaces) {
      return;
    }
    const packageId = node.domAttrs?.package;
    const focused = node.domAttrs?.focused === true ? 1 : 0;
    if (packageId == null || packageId === "") {
      bind(node.id, -1, focused);
      return;
    }
    if (typeof packageId !== "string") {
      throw new Error("PocketJS: compositor surface package must be a string id");
    }
    const handle = ops.__surfaces[packageId];
    if (handle === undefined) {
      if (getHost().strict) {
        throw new Error(`PocketJS: package ${JSON.stringify(packageId)} is not installed in this Pocket System`);
      }
      missCounters.unknownSurface++;
      bind(node.id, -1, focused);
      return;
    }
    bind(node.id, handle, focused);
  }
  function setStyleObject(node, value, prev) {
    const ops = getOps();
    const next = value ?? {};
    const before = prev ?? {};
    let changed = false;
    for (const key in next) {
      const v = next[key];
      if (before[key] === v)
        continue;
      const propId = PROP[key];
      if (propId === undefined) {
        throw new Error(`PocketJS: unknown style prop '${key}' (see spec PROP)`);
      }
      ops.setProp(node.id, propId, encodePropValue(key, v));
      changed = true;
    }
    if (changed)
      treeMutated();
  }
  function setProp(node, name, value, prev) {
    if (value === prev && name !== "style")
      return value;
    if (name === "className")
      name = "class";
    if (name !== "children" && name !== "key" && name !== "ref" && name !== "nodeRef") {
      if (value == null)
        delete domAttrs(node)[name];
      else
        domAttrs(node)[name] = value;
    }
    switch (name) {
      case "class":
        setClass(node, value);
        return value;
      case "onPress":
      case "on:press":
        registerPress(node, value);
        return value;
      case "src":
        setSrc(node, value);
        return value;
      case "sprite":
        setSpriteSrc(node, value);
        return value;
      case "frameStep":
        if (value != null)
          spriteStep(node, 1, value);
        if (node.domAttrs?.sprite != null) {
          const key = node.domAttrs.sprite;
          const meta = sprites.get(String(key));
          if (meta !== undefined) {
            getOps().setSprite(node.id, meta.handle, meta.frames, meta.cols, spriteStep(node, meta.step, value));
          } else {
            setSpriteSrc(node, key);
          }
        }
        return value;
      case "package":
      case "focused":
        setCompositorBinding(node);
        return value;
      case "style":
        setStyleObject(node, value, prev);
        return value;
      case "focusable":
        registerFocusable(node, !!value);
        return value;
      case "debugName":
        setDebugName(node, value == null ? undefined : String(value));
        return value;
      case "ref":
      case "nodeRef":
      case "key":
      case "children":
        return value;
      default:
        break;
    }
    if (name === "classList") {
      throw new Error("PocketJS: classList is not supported - use ternaries of full class literals");
    }
    if (name.startsWith("on:") || name.startsWith("bool:") || name.startsWith("prop:")) {
      throw new Error(`PocketJS: unsupported namespaced attribute '${name}'`);
    }
    throw new Error(`PocketJS: unknown property '${name}' on <${tagName(node)}>`);
  }
  function clearContainer(container) {
    for (const child of [...container.children])
      removeNode(container, child);
  }
  function tagName(node) {
    for (const key of Object.keys(NODE_TYPE)) {
      if (NODE_TYPE[key] === node.type)
        return key;
    }
    return String(node.type);
  }

  // ../../build/pocketjs-upstream/framework/src/renderer-solid.ts
  function setProperty(node, name, value, prev) {
    if (name === "ref" && typeof value === "function") {
      value(node);
      return;
    }
    setProp(node, name, value, prev);
  }
  var renderer = createRenderer({
    createElement(type) {
      const node = createElement(type);
      captureNodeRow(node);
      return node;
    },
    createTextNode,
    replaceText,
    isTextNode,
    setProperty,
    insertNode(parent, node, anchor) {
      insertNode(parent, node, anchor);
    },
    removeNode(parent, node) {
      removeNode(parent, node);
    },
    getParentNode,
    getFirstChild,
    getNextSibling
  });
  var {
    render,
    effect,
    memo: memo2,
    createComponent: createComponent2,
    createElement: createElement2,
    insert,
    spread,
    mergeProps: mergeProps2,
    use
  } = renderer;

  // ../../build/pocketjs-upstream/framework/src/display.ts
  var roots2 = null;
  function layer(width, height, overlay) {
    const node = createElement("view");
    setProp(node, "style", overlay ? {
      width,
      height,
      posType: ENUMS.PosType.Absolute,
      insetT: 0,
      insetR: 0,
      insetB: 0,
      insetL: 0,
      zIndex: 1000,
      hitPass: 1
    } : {
      width,
      height,
      overflow: ENUMS.Overflow.Hidden
    }, undefined);
    return node;
  }
  function mountAuxiliarySurface(ops) {
    const descriptor = ops.__auxiliarySurface;
    if (!descriptor) {
      roots2 = null;
      return null;
    }
    if (!Number.isInteger(descriptor.root) || descriptor.root <= 1 || !Number.isFinite(descriptor.w) || descriptor.w <= 0 || !Number.isFinite(descriptor.h) || descriptor.h <= 0) {
      throw new Error("PocketJS: invalid ui.__auxiliarySurface descriptor");
    }
    const native = adoptNativeRoot(descriptor.root, "auxiliary-root");
    const app = layer(descriptor.w, descriptor.h, false);
    const overlay = layer(descriptor.w, descriptor.h, true);
    insertNode(native, app);
    insertNode(native, overlay);
    roots2 = {
      native,
      app,
      overlay,
      viewport: Object.freeze({
        width: descriptor.w,
        height: descriptor.h
      })
    };
    return roots2;
  }
  function unmountAuxiliarySurface(ops) {
    const mounted = roots2;
    roots2 = null;
    if (!mounted)
      return;
    for (const child of mounted.native.children.splice(0)) {
      child.parent = null;
      ops.destroyNode(child.id);
    }
    releaseNativeRoot(mounted.native);
  }
  function hasAuxiliarySurface() {
    return roots2 !== null;
  }
  function getAuxiliarySurfaceRoots() {
    if (!roots2) {
      throw new Error("PocketJS: AuxiliarySurface requires a resolved display.auxiliary capability");
    }
    return roots2;
  }

  // ../../build/pocketjs-upstream/framework/src/devtools.ts
  var TAPE_CAP = 36000;
  var TOUCH_CHUNK_BITS = 7;
  var TOUCH_CHUNK_SIZE = 1 << TOUCH_CHUNK_BITS;
  function createTouchRing(surfaces = false) {
    return {
      pages: [],
      pageCount: 0,
      surfaces
    };
  }
  function writeTouch(ring, at, value, limit) {
    const page = at >>> TOUCH_CHUNK_BITS;
    const slot = at & TOUCH_CHUNK_SIZE - 1;
    let chunk = ring.pages[page];
    if (!value) {
      if (chunk?.sizes[slot]) {
        chunk.sizes[slot] = 0;
        if (--chunk.occupied === 0) {
          ring.pages[page] = undefined;
          if (--ring.pageCount === 0)
            ring.pages = [];
        }
      }
      return;
    }
    const length = Math.min(value.length, limit);
    if (!chunk) {
      let stride = 1;
      while (stride < length)
        stride *= 2;
      chunk = ring.pages[page] = {
        sizes: new Uint8Array(TOUCH_CHUNK_SIZE),
        words: ring.surfaces ? new Uint8Array(TOUCH_CHUNK_SIZE * stride) : new Float64Array(TOUCH_CHUNK_SIZE * stride),
        stride,
        occupied: 0
      };
      ring.pageCount++;
    } else if (length > chunk.stride) {
      let stride = chunk.stride;
      while (stride < length)
        stride *= 2;
      const words = ring.surfaces ? new Uint8Array(TOUCH_CHUNK_SIZE * stride) : new Float64Array(TOUCH_CHUNK_SIZE * stride);
      for (let frame = 0;frame < TOUCH_CHUNK_SIZE; frame++) {
        for (let word = 0;word < chunk.sizes[frame] - 1; word++) {
          words[frame * stride + word] = chunk.words[frame * chunk.stride + word];
        }
      }
      chunk.words = words;
      chunk.stride = stride;
    }
    if (!chunk.sizes[slot])
      chunk.occupied++;
    chunk.sizes[slot] = length + 1;
    for (let word = 0;word < length; word++) {
      chunk.words[slot * chunk.stride + word] = ring.surfaces ? value[word] === 1 ? 1 : 0 : value[word];
    }
  }
  function readTouch(ring, at) {
    const chunk = ring.pages[at >>> TOUCH_CHUNK_BITS];
    const slot = at & TOUCH_CHUNK_SIZE - 1;
    const size = chunk?.sizes[slot];
    if (!size)
      return null;
    const value = new Array(size - 1);
    for (let word = 0;word < value.length; word++)
      value[word] = chunk.words[slot * chunk.stride + word];
    return value;
  }
  var TREE_THROTTLE = 30;
  var STATS_EVERY = 30;
  var state = {
    ops: null,
    transport: null,
    app: undefined,
    frame: 0,
    tape: new Uint16Array(TAPE_CAP),
    tapeAnalog: new Uint16Array(TAPE_CAP),
    tapeRightAnalog: null,
    tapeTouch: null,
    tapeTouchSurfaces: null,
    tapeAxes: null,
    tapeMotion: null,
    tapeStart: 0,
    tapeLen: 0,
    tapeFirstFrame: 0,
    replayMasks: null,
    replayAnalog: null,
    replayRightAnalog: null,
    replayTouch: null,
    replayTouchSurfaces: null,
    replayAxes: null,
    replayMotion: null,
    replayAt: 0,
    paused: false,
    stepQueued: 0,
    inspectReportId: null,
    inspectAskedAt: 0,
    treeDirty: true,
    treeSentAt: -TREE_THROTTLE,
    saidHello: false,
    panelAttached: false,
    hostCalls: 0
  };
  function initDevtools(ops) {
    const g = globalThis;
    if (!g.console)
      g.console = {
        log() {},
        warn() {},
        error() {}
      };
    state.ops = ops;
    state.frame = 0;
    state.tapeStart = 0;
    state.tapeLen = 0;
    state.tapeFirstFrame = 0;
    state.tapeTouch = null;
    state.tapeRightAnalog = null;
    state.tapeTouchSurfaces = null;
    state.tapeAxes = null;
    state.tapeMotion = null;
    state.replayMasks = null;
    state.replayAnalog = null;
    state.replayRightAnalog = null;
    state.replayTouch = null;
    state.replayTouchSurfaces = null;
    state.replayAxes = null;
    state.replayMotion = null;
    state.paused = false;
    state.stepQueued = 0;
    state.inspectReportId = null;
    state.inspectAskedAt = 0;
    state.treeDirty = true;
    state.treeSentAt = -TREE_THROTTLE;
    state.saidHello = false;
    state.panelAttached = false;
    state.hostCalls = 0;
    state.app = globalThis.__pocketApp;
    const injected = globalThis.__pocketDevtoolsTransport;
    if (injected) {
      state.transport = injected;
    } else if (ops.__dbgActive?.() && ops.__dbgPoll && ops.__dbgSend) {
      state.transport = {
        send: (l) => ops.__dbgSend(l),
        recv: () => ops.__dbgPoll(),
        everyFrames: 10,
        listening: ops.__dbgConnected ? () => ops.__dbgConnected() : undefined
      };
    } else {
      state.transport = null;
    }
    if (state.transport) {
      setTreeMutationHook(() => {
        state.treeDirty = true;
      });
    } else {
      setTreeMutationHook(null);
    }
    bridgeConsole();
    globalThis.__pocketDevtools = api;
  }
  function wrapFrameHandler(h) {
    return (buttons, analogArg, touchArg, hitsArg, touchSurfacesArg, rightAnalogArg, axisDeltasArg, motionArg, textArg) => {
      state.hostCalls++;
      if (state.transport) {
        pollTransport();
        flushInspectReport();
      }
      let mask = buttons;
      let analog = analogArg === undefined ? ANALOG_CENTER : analogArg & 65535;
      let rightAnalog = rightAnalogArg === undefined ? ANALOG_CENTER : rightAnalogArg & 65535;
      let touch = touchArg;
      let hits = hitsArg;
      let touchSurfaces = touchSurfacesArg;
      let axisDeltas = axisDeltasArg;
      let motion = motionArg;
      if (state.replayMasks) {
        if (state.replayAt < state.replayMasks.length) {
          if (state.paused && state.stepQueued <= 0)
            return;
          axisDeltas = state.replayAxes?.[state.replayAt] ?? EMPTY_AXIS_DELTAS;
          motion = state.replayMotion?.[state.replayAt] ?? null;
          mask = state.replayMasks[state.replayAt];
          analog = state.replayAnalog ? state.replayAnalog[state.replayAt] : ANALOG_CENTER;
          rightAnalog = state.replayRightAnalog ? state.replayRightAnalog[state.replayAt] : ANALOG_CENTER;
          touch = state.replayTouch ? state.replayTouch[state.replayAt] : undefined;
          touchSurfaces = state.replayTouchSurfaces ? state.replayTouchSurfaces[state.replayAt] : undefined;
          hits = undefined;
          state.replayAt++;
        } else {
          state.replayMasks = null;
          state.replayAnalog = null;
          state.replayRightAnalog = null;
          state.replayTouch = null;
          state.replayTouchSurfaces = null;
          state.replayAxes = null;
          state.replayMotion = null;
          send({
            t: "replayDone",
            frame: state.frame
          });
        }
      }
      if (state.paused) {
        if (state.stepQueued <= 0)
          return;
        state.stepQueued--;
        state.ops?.debugStep?.();
      }
      axisDeltas = __takeAxisDeltas(axisDeltas);
      motion = __takeMotionState(motion);
      recordMask(mask, analog, touch, touchSurfaces, rightAnalog, axisDeltas, motion);
      state.frame++;
      try {
        h(mask, analog, touch, hits, touchSurfaces, rightAnalog, axisDeltas, motion, textArg);
      } catch (e) {
        send({
          t: "error",
          frame: state.frame,
          message: e instanceof Error ? e.message : String(e),
          stack: e instanceof Error ? e.stack : undefined
        });
        throw e;
      }
      if (state.transport)
        afterFrame();
    };
  }
  function recordMask(mask, analog, touch, touchSurfaces, rightAnalog, axisDeltas, motion) {
    const axes = axisDeltas?.length ? axisDeltas.map((value) => ({
      ...value
    })) : null;
    if (axes && !state.tapeAxes)
      state.tapeAxes = new Array(TAPE_CAP).fill(null);
    if (motion && !state.tapeMotion)
      state.tapeMotion = new Array(TAPE_CAP).fill(null);
    const right = rightAnalog ?? ANALOG_CENTER;
    if (right !== ANALOG_CENTER && !state.tapeRightAnalog)
      state.tapeRightAnalog = new Uint16Array(TAPE_CAP).fill(ANALOG_CENTER);
    const contacts = touch && touch.length > 0 ? touch : null;
    const contactCount = Math.min(contacts?.length ?? 0, 16);
    if (contacts && !state.tapeTouch) {
      state.tapeTouch = createTouchRing();
    }
    const surfaces = contacts && touchSurfaces ? touchSurfaces : null;
    if (surfaces && !state.tapeTouchSurfaces) {
      for (let word = 0;word < Math.min(surfaces.length, contactCount); word++) {
        if (surfaces[word] === 1) {
          state.tapeTouchSurfaces = createTouchRing(true);
          break;
        }
      }
    }
    if (state.tapeLen < TAPE_CAP) {
      const at = (state.tapeStart + state.tapeLen) % TAPE_CAP;
      state.tape[at] = mask;
      state.tapeAnalog[at] = analog;
      if (state.tapeRightAnalog)
        state.tapeRightAnalog[at] = right;
      if (state.tapeTouch)
        writeTouch(state.tapeTouch, at, contacts, contactCount);
      if (state.tapeTouchSurfaces)
        writeTouch(state.tapeTouchSurfaces, at, surfaces, contactCount);
      if (state.tapeAxes)
        state.tapeAxes[at] = axes;
      if (state.tapeMotion)
        state.tapeMotion[at] = motion ?? null;
      state.tapeLen++;
    } else {
      state.tape[state.tapeStart] = mask;
      state.tapeAnalog[state.tapeStart] = analog;
      if (state.tapeRightAnalog)
        state.tapeRightAnalog[state.tapeStart] = right;
      if (state.tapeTouch)
        writeTouch(state.tapeTouch, state.tapeStart, contacts, contactCount);
      if (state.tapeTouchSurfaces)
        writeTouch(state.tapeTouchSurfaces, state.tapeStart, surfaces, contactCount);
      if (state.tapeAxes)
        state.tapeAxes[state.tapeStart] = axes;
      if (state.tapeMotion)
        state.tapeMotion[state.tapeStart] = motion ?? null;
      state.tapeStart = (state.tapeStart + 1) % TAPE_CAP;
      state.tapeFirstFrame++;
    }
  }
  function rlePairs(ring) {
    const out = [];
    for (let i = 0;i < state.tapeLen; i++) {
      const v = ring[(state.tapeStart + i) % TAPE_CAP];
      const last = out[out.length - 1];
      if (last && last[0] === v)
        last[1]++;
      else
        out.push([v, 1]);
    }
    return out;
  }
  function exportTape() {
    const tape = {
      v: 1,
      app: state.app,
      frames: state.tapeLen,
      masks: rlePairs(state.tape),
      startFrame: state.tapeFirstFrame
    };
    const analog = rlePairs(state.tapeAnalog);
    if (analog.length > 1 || analog.length === 1 && analog[0][0] !== ANALOG_CENTER) {
      tape.analog = analog;
    }
    if (state.tapeRightAnalog) {
      const right = rlePairs(state.tapeRightAnalog);
      if (right.some(([value]) => value !== ANALOG_CENTER))
        tape.rightAnalog = right;
    }
    if (state.tapeTouch) {
      const touch = [];
      for (let i = 0;i < state.tapeLen; i++) {
        const contacts = readTouch(state.tapeTouch, (state.tapeStart + i) % TAPE_CAP);
        if (contacts)
          touch.push([i, contacts]);
      }
      if (touch.length > 0) {
        tape.v = 2;
        tape.touch = touch;
      }
    }
    if (state.tapeTouchSurfaces) {
      const touchSurfaces = [];
      for (let i = 0;i < state.tapeLen; i++) {
        const surfaces = readTouch(state.tapeTouchSurfaces, (state.tapeStart + i) % TAPE_CAP);
        if (surfaces)
          touchSurfaces.push([i, surfaces]);
      }
      if (touchSurfaces.length > 0) {
        tape.v = 3;
        tape.touchSurfaces = touchSurfaces;
      }
    }
    if (state.tapeAxes) {
      const axes = [];
      for (let i = 0;i < state.tapeLen; i++) {
        const deltas = state.tapeAxes[(state.tapeStart + i) % TAPE_CAP];
        if (deltas)
          axes.push([i, deltas.map((value) => ({
            ...value
          }))]);
      }
      if (axes.length) {
        tape.v = 4;
        tape.axes = axes;
      }
    }
    if (state.tapeMotion) {
      const motion = [];
      for (let i = 0;i < state.tapeLen; i++) {
        const estimate = state.tapeMotion[(state.tapeStart + i) % TAPE_CAP];
        if (estimate)
          motion.push([i, __copyMotionState(estimate)]);
      }
      if (motion.length) {
        tape.v = 5;
        tape.motion = motion;
      }
    }
    return tape;
  }
  function expandPairs(pairs, fill, total) {
    const out = new Uint16Array(total).fill(fill);
    let at = 0;
    for (const [v, n] of pairs) {
      out.fill(v, at, Math.min(at + n, total));
      at += n;
    }
    return out;
  }
  function expandTape(tape) {
    let total = 0;
    for (const [, n] of tape.masks)
      total += n;
    return expandPairs(tape.masks, 0, total);
  }
  function expandTapeAnalog(tape) {
    let total = 0;
    for (const [, n] of tape.masks)
      total += n;
    return expandPairs(tape.analog ?? [], ANALOG_CENTER, total);
  }
  function expandTapeRightAnalog(tape) {
    return expandPairs(tape.rightAnalog ?? [], ANALOG_CENTER, tape.masks.reduce((n, [, count]) => n + count, 0));
  }
  function expandTapeTouch(tape) {
    let total = 0;
    for (const [, n] of tape.masks)
      total += n;
    const out = new Array(total).fill(undefined);
    for (const [frame, contacts] of tape.touch ?? []) {
      if (frame >= 0 && frame < total)
        out[frame] = contacts;
    }
    return out;
  }
  function expandTapeAxes(tape) {
    const total = tape.masks.reduce((n, [, count]) => n + count, 0);
    const out = new Array(total).fill(undefined);
    for (const [frame, deltas] of tape.axes ?? []) {
      if (Number.isInteger(frame) && frame >= 0 && frame < total)
        out[frame] = deltas.map((value) => ({
          ...value
        }));
    }
    return out;
  }
  function expandTapeMotion(tape) {
    const total = tape.masks.reduce((n, [, count]) => n + count, 0);
    const out = new Array(total).fill(undefined);
    for (const [frame, estimate] of tape.motion ?? []) {
      if (Number.isInteger(frame) && frame >= 0 && frame < total)
        out[frame] = __copyMotionState(estimate);
    }
    return out;
  }
  function expandTapeTouchSurfaces(tape) {
    let total = 0;
    for (const [, n] of tape.masks)
      total += n;
    const out = new Array(total).fill(undefined);
    for (const [frame, surfaces] of tape.touchSurfaces ?? []) {
      if (frame >= 0 && frame < total)
        out[frame] = surfaces;
    }
    return out;
  }
  function send(msg) {
    try {
      state.transport?.send(JSON.stringify(msg));
    } catch {}
  }
  function pollTransport() {
    const t = state.transport;
    const every = t.everyFrames ?? 1;
    if (every > 1 && state.hostCalls % every !== 0)
      return;
    if (!state.saidHello) {
      state.saidHello = true;
      send({
        t: "hello",
        app: state.app,
        host: hostKind(),
        frame: state.frame
      });
    }
    for (let guard = 0;guard < 64; guard++) {
      const chunk = t.recv();
      if (!chunk)
        break;
      for (const line of chunk.split(`
`)) {
        if (line.trim())
          handleMessage(line);
      }
    }
  }
  function handleMessage(line) {
    let msg;
    try {
      msg = JSON.parse(line);
    } catch {
      return;
    }
    const ops = state.ops;
    switch (msg.t) {
      case "inspect": {
        const id = typeof msg.id === "number" ? msg.id : 0;
        ops?.debugInspect?.(id);
        state.inspectReportId = id || null;
        state.inspectAskedAt = state.hostCalls;
        if (!id)
          send({
            t: "inspect",
            id: 0,
            rect: null
          });
        break;
      }
      case "pause":
        state.paused = true;
        state.stepQueued = 0;
        ops?.debugPause?.(true);
        sendStats();
        break;
      case "resume":
        state.paused = false;
        ops?.debugPause?.(false);
        sendStats();
        break;
      case "step":
        state.stepQueued += typeof msg.n === "number" && msg.n > 0 ? msg.n : 1;
        break;
      case "getTree":
        sendTree();
        break;
      case "eval": {
        let ok = true;
        let value;
        try {
          value = fmt((0, eval)(String(msg.code)));
        } catch (e) {
          ok = false;
          value = e instanceof Error ? `${e.name}: ${e.message}` : String(e);
        }
        send({
          t: "evalResult",
          id: msg.id,
          ok,
          value
        });
        break;
      }
      case "dumpTape":
        send({
          t: "tape",
          tape: exportTape()
        });
        break;
      case "devStats": {
        let data = null;
        const raw = ops?.debugStats?.();
        if (raw) {
          try {
            data = JSON.parse(raw);
          } catch {
            data = null;
          }
        }
        send({
          t: "devStats",
          frame: state.frame,
          data
        });
        break;
      }
      case "screenshot": {
        if (ops?.__dbgShot?.()) {
          send({
            t: "screenshotRaw",
            file: "shot.raw",
            w: 480,
            h: 272,
            stride: 512,
            frame: state.frame
          });
        } else {
          send({
            t: "log",
            level: "warn",
            args: ["screenshot: not supported on this host"]
          });
        }
        break;
      }
      case "replay": {
        const tape = msg.tape;
        if (tape && Array.isArray(tape.masks)) {
          state.replayMotion = tape.motion ? expandTapeMotion(tape) : null;
          state.replayAxes = tape.axes ? expandTapeAxes(tape) : null;
          state.replayMasks = expandTape(tape);
          state.replayAnalog = tape.analog ? expandTapeAnalog(tape) : null;
          state.replayRightAnalog = tape.rightAnalog ? expandTapeRightAnalog(tape) : null;
          state.replayTouch = tape.touch ? expandTapeTouch(tape) : null;
          state.replayTouchSurfaces = tape.touchSurfaces ? expandTapeTouchSurfaces(tape) : null;
          state.replayAt = 0;
        }
        break;
      }
      default:
        break;
    }
  }
  function panelListening() {
    const listening = state.transport?.listening;
    return !listening || listening();
  }
  function afterFrame() {
    if (!panelListening()) {
      state.panelAttached = false;
      return;
    }
    if (!state.panelAttached) {
      state.panelAttached = true;
      state.treeDirty = true;
      state.treeSentAt = -TREE_THROTTLE;
    }
    if (state.treeDirty && state.frame - state.treeSentAt >= TREE_THROTTLE) {
      sendTree();
    }
    if (state.frame % STATS_EVERY === 0)
      sendStats();
  }
  function flushInspectReport() {
    const id = state.inspectReportId;
    if (id == null)
      return;
    const ops = state.ops;
    if (!ops?.debugRectXY || !ops.debugRectWH) {
      state.inspectReportId = null;
      return;
    }
    const xy = ops.debugRectXY();
    if (xy === -1) {
      if (state.hostCalls - state.inspectAskedAt > 60) {
        state.inspectReportId = null;
        send({
          t: "inspect",
          id,
          rect: null
        });
      }
      return;
    }
    const wh = ops.debugRectWH();
    state.inspectReportId = null;
    send({
      t: "inspect",
      id,
      rect: [xy << 16 >> 16, xy >> 16, wh & 65535, wh >> 16 & 65535]
    });
  }
  function sendStats() {
    send({
      t: "stats",
      frame: state.frame,
      nodes: countMountedNodes(),
      tapeLen: state.tapeLen,
      paused: state.paused
    });
  }
  function sendTree() {
    state.treeDirty = false;
    state.treeSentAt = state.frame;
    send({
      t: "tree",
      frame: state.frame,
      root: serializeMountedTree()
    });
  }
  function isTreeMirror(value) {
    if (value == null || typeof value !== "object")
      return false;
    const candidate = value;
    return typeof candidate.id === "number" && typeof candidate.type === "number";
  }
  function forEachTreeMirror(value, visit) {
    if (Array.isArray(value)) {
      for (const entry of value)
        forEachTreeMirror(entry, visit);
      return;
    }
    if (isTreeMirror(value)) {
      visit(value);
      return;
    }
    if (value != null && typeof value === "object") {
      const nodes = value.nodes;
      if (nodes !== undefined)
        forEachTreeMirror(nodes, visit);
    }
  }
  function forEachTreeChild(node, visit) {
    const children = Array.isArray(node.children) ? node.children : [];
    for (const child of children)
      forEachTreeMirror(child, visit);
  }
  function serializeNode(node) {
    const out = {
      i: node.id,
      t: node.domTag ?? String(node.type)
    };
    if (node.debugName)
      out.n = node.debugName;
    const cls = node.domAttrs?.class;
    if (typeof cls === "string" && cls)
      out.c = cls;
    if (node.text)
      out.x = node.text.length > 80 ? node.text.slice(0, 79) + "…" : node.text;
    const kids = [];
    forEachTreeChild(node, (child) => {
      if (child.domNodeType === 8)
        return;
      kids.push(serializeNode(child));
    });
    if (kids.length)
      out.k = kids;
    return out;
  }
  function namedSurfaceRoot(node, name) {
    const out = serializeNode(node);
    if (!out.n)
      out.n = name;
    return out;
  }
  function serializeMountedTree() {
    if (!hasAuxiliarySurface())
      return serializeNode(rootMirror);
    return {
      i: 0,
      t: "surfaces",
      v: 1,
      n: "Displays",
      k: [namedSurfaceRoot(rootMirror, "Primary display"), namedSurfaceRoot(getAuxiliarySurfaceRoots().native, "Auxiliary display")]
    };
  }
  function countNodes(node) {
    let n = 1;
    forEachTreeChild(node, (child) => {
      n += countNodes(child);
    });
    return n;
  }
  function countMountedNodes() {
    let count = countNodes(rootMirror);
    if (hasAuxiliarySurface())
      count += countNodes(getAuxiliarySurfaceRoots().native);
    return count;
  }
  function bridgeConsole() {
    const g = globalThis;
    if (!g.console)
      g.console = {};
    const c = g.console;
    const bridge = state.transport ? {
      send,
      listening: panelListening
    } : {
      send() {},
      listening: () => false
    };
    if (c.__pocketBridge) {
      c.__pocketBridge.current = bridge;
      return;
    }
    if (!state.transport)
      return;
    const holder = {
      current: bridge
    };
    c.__pocketBridge = holder;
    for (const level of ["log", "warn", "error"]) {
      const original = c[level];
      c[level] = (...args) => {
        if (holder.current.listening())
          holder.current.send({
            t: "log",
            level,
            args: args.map((a) => fmt(a))
          });
        original?.apply(c, args);
      };
    }
  }
  function fmt(v, depth = 0) {
    if (v === undefined)
      return "undefined";
    if (v === null)
      return "null";
    const t = typeof v;
    if (t === "string") {
      const s = v;
      return depth === 0 ? clip(s) : JSON.stringify(clip(s));
    }
    if (t === "number" || t === "boolean" || t === "bigint")
      return String(v);
    if (t === "function") {
      const name = v.name;
      return name ? `[function ${name}]` : "[function]";
    }
    if (depth >= 3)
      return Array.isArray(v) ? "[…]" : "{…}";
    if (Array.isArray(v)) {
      const items = v.slice(0, 20).map((x) => fmt(x, depth + 1));
      if (v.length > 20)
        items.push(`… ${v.length - 20} more`);
      return `[${items.join(", ")}]`;
    }
    if (v instanceof Error)
      return `${v.name}: ${v.message}`;
    const entries = Object.entries(v).slice(0, 20);
    const body = entries.map(([k, x]) => `${k}: ${fmt(x, depth + 1)}`).join(", ");
    return `{${body}}`;
  }
  function clip(s) {
    return s.length > 200 ? s.slice(0, 199) + "…" : s;
  }
  function hostKind() {
    const ops = state.ops;
    if (typeof ops?.__host === "string")
      return ops.__host;
    if (ops?.__textures !== undefined)
      return "psp";
    if (typeof globalThis.document !== "undefined")
      return "web";
    return "headless";
  }
  var api = {
    get frame() {
      return state.frame;
    },
    dumpTape: () => exportTape(),
    replay: (tape) => {
      state.replayMotion = tape.motion ? expandTapeMotion(tape) : null;
      state.replayAxes = tape.axes ? expandTapeAxes(tape) : null;
      state.replayMasks = expandTape(tape);
      state.replayAnalog = tape.analog ? expandTapeAnalog(tape) : null;
      state.replayRightAnalog = tape.rightAnalog ? expandTapeRightAnalog(tape) : null;
      state.replayTouch = tape.touch ? expandTapeTouch(tape) : null;
      state.replayTouchSurfaces = tape.touchSurfaces ? expandTapeTouchSurfaces(tape) : null;
      state.replayAt = 0;
    }
  };
  // ../../build/pocketjs-upstream/framework/src/overlay.ts
  var overlayRoot = null;
  function setOverlayRoot(root) {
    overlayRoot = root;
  }

  // ../../build/pocketjs-upstream/framework/src/styles.ts
  var verbatim = new Map;
  var sortedAlias = new Map;
  function normalize(cls) {
    return cls.trim().replace(/\s+/g, " ");
  }
  function sortTokens(normalized) {
    return normalized.split(" ").sort().join(" ");
  }
  var ALIAS_AMBIGUOUS = -1;
  function registerStyles(table) {
    for (const key of Object.keys(table)) {
      const id = table[key];
      const norm = normalize(key);
      verbatim.set(norm, id);
      const sorted = sortTokens(norm);
      const prev = sortedAlias.get(sorted);
      sortedAlias.set(sorted, prev !== undefined && prev !== id ? ALIAS_AMBIGUOUS : id);
    }
  }
  function resolveStyle(cls) {
    const norm = normalize(cls);
    const hit = verbatim.get(norm);
    if (hit !== undefined)
      return hit;
    const alias = sortedAlias.get(sortTokens(norm));
    return alias === ALIAS_AMBIGUOUS ? undefined : alias;
  }

  // ../../build/pocketjs-upstream/framework/src/touch.ts
  var LEGACY_COORD_BITS = 9;
  var LEGACY_COORD_MASK = (1 << LEGACY_COORD_BITS) - 1;
  var LEGACY_ID_SHIFT = LEGACY_COORD_BITS * 2;
  var WIDE_MARKER = 2147483648;
  var WIDE_COORD_BITS = 10;
  var WIDE_COORD_MASK = (1 << WIDE_COORD_BITS) - 1;
  var WIDE_ID_SHIFT = WIDE_COORD_BITS * 2;
  var CANCEL_MARKER = 1073741824;
  var EMPTY = Object.freeze([]);
  var primarySnapshot = EMPTY;
  var auxiliarySnapshot = EMPTY;
  var allSnapshot = EMPTY;
  var cancelledSnapshot = EMPTY;
  function __setTouches(packed, hits, surfaces) {
    cancelledSnapshot = EMPTY;
    if (!packed || packed.length === 0) {
      primarySnapshot = EMPTY;
      auxiliarySnapshot = EMPTY;
      allSnapshot = EMPTY;
      return;
    }
    const all = [], cancelled = [];
    for (let index = 0;index < Math.min(packed.length, 16); index++) {
      const value = packed[index];
      const output = value & CANCEL_MARKER ? cancelled : all;
      if (output.length >= 8)
        continue;
      const wide = (value & WIDE_MARKER) !== 0;
      const coordBits = wide ? WIDE_COORD_BITS : LEGACY_COORD_BITS;
      const coordMask = wide ? WIDE_COORD_MASK : LEGACY_COORD_MASK;
      const idShift = wide ? WIDE_ID_SHIFT : LEGACY_ID_SHIFT;
      output.push(Object.freeze({
        surface: surfaces?.[index] === 1 ? "auxiliary" : "primary",
        id: value >>> idShift & 255,
        x: value & coordMask,
        y: value >>> coordBits & coordMask,
        hit: hits?.[index]
      }));
    }
    cancelledSnapshot = Object.freeze(cancelled);
    allSnapshot = Object.freeze(all);
    primarySnapshot = Object.freeze(all.filter((contact) => contact.surface === "primary"));
    auxiliarySnapshot = Object.freeze(all.filter((contact) => contact.surface === "auxiliary"));
  }
  function __cancelledTouches() {
    return cancelledSnapshot;
  }
  function __allTouches() {
    return allSnapshot;
  }
  function __resetTouches() {
    cancelledSnapshot = EMPTY;
    primarySnapshot = EMPTY;
    auxiliarySnapshot = EMPTY;
    allSnapshot = EMPTY;
  }

  // ../../build/pocketjs-upstream/framework/src/gesture-core.ts
  var MAX_TRACKS = 8;
  var HIST = 8;
  var VELOCITY_WINDOW = 3;
  var DEFAULT_TAP_SLOP = 8;
  var DEFAULT_PAN_SLOP = 6;
  var DEFAULT_PINCH_SLOP = 10;
  var DEFAULT_LONG_PRESS_S = 0.5;
  var OBSERVING = 1;
  var TAP_DEAD = 2;
  var LONGPRESS_FIRED = 4;
  var PANNING = 8;
  var recognizers = [];
  var blockDepth = 0;
  var liveCount = 0;
  var tracks = Array.from({
    length: MAX_TRACKS
  }, (_, slot) => ({
    slot,
    used: false,
    present: false,
    surface: "primary",
    id: 0,
    x: 0,
    y: 0,
    startX: 0,
    startY: 0,
    dx: 0,
    dy: 0,
    fdx: 0,
    fdy: 0,
    vx: 0,
    vy: 0,
    downFrame: 0,
    frames: 0,
    histX: new Int16Array(HIST),
    histY: new Int16Array(HIST),
    histHead: 0,
    histLen: 0,
    owners: [],
    claimedBy: null
  }));
  var pinchScratch = {
    ax: 0,
    ay: 0,
    bx: 0,
    by: 0,
    cx: 0,
    cy: 0,
    span: 0,
    startSpan: 0,
    dspan: 0,
    fdspan: 0
  };
  function withinSubtree(node, ancestor) {
    let n = node;
    while (n) {
      if (n === ancestor)
        return true;
      n = n.parent;
    }
    return false;
  }
  function regionMatches(rec, x, y, hitBox) {
    if ((rec.opts.surface ?? "primary") !== hitBox.surface)
      return false;
    const region = rec.opts.region;
    if (!region)
      return true;
    const target = region.node?.();
    if (target) {
      if (hitBox.hit === undefined) {
        hitBox.hit = resolveTouchHit(x, y, hitBox.fact, hitBox.surface);
      }
      const hit = hitBox.hit;
      if (hit)
        return withinSubtree(hit, target);
    }
    const r = region.rect?.();
    if (!r)
      return false;
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
  }
  function hasPinchHandlers(rec) {
    return !!(rec.opts.onPinchStart || rec.opts.onPinchMove || rec.opts.onPinchEnd);
  }
  function pinchSpan(axis, a, b) {
    if (axis === "y") {
      const d = a.y - b.y;
      return d < 0 ? -d : d;
    }
    if (axis === "x") {
      const d = a.x - b.x;
      return d < 0 ? -d : d;
    }
    const dx = a.x - b.x;
    const dy = a.y - b.y;
    return Math.sqrt(dx * dx + dy * dy);
  }
  function fillPinchScratch(rec, a, b) {
    const axis = rec.opts.axis ?? "any";
    const span = pinchSpan(axis, a, b);
    pinchScratch.ax = a.x;
    pinchScratch.ay = a.y;
    pinchScratch.bx = b.x;
    pinchScratch.by = b.y;
    pinchScratch.cx = (a.x + b.x) / 2;
    pinchScratch.cy = (a.y + b.y) / 2;
    pinchScratch.span = span;
    pinchScratch.startSpan = rec.baseSpan;
    pinchScratch.dspan = span - rec.baseSpan;
    pinchScratch.fdspan = span - rec.prevSpan;
  }
  function clearPinchPair(rec) {
    rec.pairA = -1;
    rec.pairB = -1;
  }
  function endPinch(rec) {
    if (rec.pinchA < 0)
      return;
    const a = tracks[rec.pinchA];
    const b = tracks[rec.pinchB];
    rec.pinchA = -1;
    rec.pinchB = -1;
    clearPinchPair(rec);
    fillPinchScratch(rec, a, b);
    rec.opts.onPinchEnd?.(pinchScratch);
  }
  function fireCancel(rec, t) {
    rec.flags[t.slot] = 0;
    if (t.claimedBy === rec)
      t.claimedBy = null;
    if (rec.pinchA === t.slot || rec.pinchB === t.slot)
      endPinch(rec);
    rec.opts.onCancel?.(t);
  }
  function claim(t, winner) {
    t.claimedBy = winner;
    for (const o of t.owners) {
      if (o !== winner && o.flags[t.slot] & OBSERVING)
        fireCancel(o, t);
    }
  }
  function releaseTrack(t) {
    for (const o of t.owners)
      o.flags[t.slot] = 0;
    t.owners.length = 0;
    t.claimedBy = null;
    t.used = false;
    liveCount--;
  }
  function beginTrack(t, surface, id, x, y, fact) {
    t.used = true;
    t.present = true;
    t.hit = fact;
    t.surface = surface;
    t.id = id;
    t.x = x;
    t.y = y;
    t.startX = x;
    t.startY = y;
    t.dx = 0;
    t.dy = 0;
    t.fdx = 0;
    t.fdy = 0;
    t.vx = 0;
    t.vy = 0;
    t.downFrame = virtualFrame();
    t.frames = 0;
    t.histX[0] = x;
    t.histY[0] = y;
    t.histHead = 1;
    t.histLen = 1;
    t.owners.length = 0;
    t.claimedBy = null;
    liveCount++;
    const hitBox = {
      hit: undefined,
      fact,
      surface
    };
    for (let i = recognizers.length - 1;i >= 0; i--) {
      const rec = recognizers[i];
      if (rec.disposed)
        continue;
      if (blockDepth > 0 && !rec.opts.allowWhenBlocked)
        continue;
      if (!regionMatches(rec, x, y, hitBox))
        continue;
      rec.flags[t.slot] = OBSERVING;
      t.owners.push(rec);
    }
    for (const o of t.owners)
      o.opts.onDown?.(t);
  }
  function updateTrack(t, x, y) {
    t.present = true;
    t.fdx = x - t.x;
    t.fdy = y - t.y;
    t.x = x;
    t.y = y;
    t.dx = x - t.startX;
    t.dy = y - t.startY;
    t.frames++;
    t.histX[t.histHead] = x;
    t.histY[t.histHead] = y;
    t.histHead = (t.histHead + 1) % HIST;
    if (t.histLen < HIST)
      t.histLen++;
    const k = Math.min(VELOCITY_WINDOW, t.histLen - 1);
    if (k <= 0) {
      t.vx = 0;
      t.vy = 0;
      return;
    }
    const hz = simulationHz();
    const last = (t.histHead - 1 + HIST) % HIST;
    const prev = (last - k + HIST) % HIST;
    t.vx = (t.histX[last] - t.histX[prev]) * hz / k;
    t.vy = (t.histY[last] - t.histY[prev]) * hz / k;
  }
  function recognizePinches() {
    for (let i = recognizers.length - 1;i >= 0; i--) {
      const rec = recognizers[i];
      if (rec.disposed || !hasPinchHandlers(rec))
        continue;
      if (rec.pinchA >= 0) {
        const a = tracks[rec.pinchA];
        const b = tracks[rec.pinchB];
        fillPinchScratch(rec, a, b);
        rec.opts.onPinchMove?.(pinchScratch);
        rec.prevSpan = pinchScratch.span;
        continue;
      }
      let a = null;
      let b = null;
      for (const t of tracks) {
        if (!t.used || !t.present || t.claimedBy)
          continue;
        if (!(rec.flags[t.slot] & OBSERVING))
          continue;
        if (!a)
          a = t;
        else if (!b)
          b = t;
        else
          break;
      }
      if (!a || !b) {
        clearPinchPair(rec);
        continue;
      }
      const axis = rec.opts.axis ?? "any";
      if (rec.pairA !== a.slot || rec.pairB !== b.slot) {
        rec.pairA = a.slot;
        rec.pairB = b.slot;
        rec.baseSpan = pinchSpan(axis, a, b);
        rec.baseCx = (a.x + b.x) / 2;
        rec.baseCy = (a.y + b.y) / 2;
        rec.prevSpan = rec.baseSpan;
        continue;
      }
      const span = pinchSpan(axis, a, b);
      const dspan = span - rec.baseSpan;
      const absDspan = dspan < 0 ? -dspan : dspan;
      const slop = rec.opts.pinchSlop ?? DEFAULT_PINCH_SLOP;
      if (absDspan <= slop)
        continue;
      const cx = (a.x + b.x) / 2;
      const cy = (a.y + b.y) / 2;
      const travelX = cx - rec.baseCx;
      const travelY = cy - rec.baseCy;
      const travel = axis === "y" ? travelY < 0 ? -travelY : travelY : axis === "x" ? travelX < 0 ? -travelX : travelX : Math.sqrt(travelX * travelX + travelY * travelY);
      if (absDspan < travel)
        continue;
      rec.pinchA = a.slot;
      rec.pinchB = b.slot;
      rec.flags[a.slot] |= TAP_DEAD;
      rec.flags[b.slot] |= TAP_DEAD;
      claim(a, rec);
      claim(b, rec);
      fillPinchScratch(rec, a, b);
      rec.opts.onPinchStart?.(pinchScratch);
      rec.prevSpan = pinchScratch.span;
    }
  }
  function recognize(t) {
    const moved = t.fdx !== 0 || t.fdy !== 0;
    const adx = t.dx < 0 ? -t.dx : t.dx;
    const ady = t.dy < 0 ? -t.dy : t.dy;
    for (const rec of t.owners) {
      const f = rec.flags[t.slot];
      if (!(f & OBSERVING))
        continue;
      if (moved)
        rec.opts.onMove?.(t);
      const slop = rec.opts.tapSlop ?? DEFAULT_TAP_SLOP;
      if (!(f & TAP_DEAD) && (adx > slop || ady > slop)) {
        rec.flags[t.slot] |= TAP_DEAD;
      }
    }
    if (!t.claimedBy) {
      for (const rec of t.owners) {
        const f = rec.flags[t.slot];
        if (!(f & OBSERVING) || f & (TAP_DEAD | LONGPRESS_FIRED))
          continue;
        if (!rec.opts.onLongPress)
          continue;
        const deadline = Math.max(1, Math.round((rec.opts.longPressSeconds ?? DEFAULT_LONG_PRESS_S) * simulationHz()));
        if (t.frames < deadline)
          continue;
        rec.flags[t.slot] |= LONGPRESS_FIRED;
        rec.opts.onLongPress(t);
        claim(t, rec);
        break;
      }
    }
    if (!t.claimedBy) {
      for (const rec of t.owners) {
        const f = rec.flags[t.slot];
        if (!(f & OBSERVING) || f & PANNING)
          continue;
        if (!rec.opts.onPanStart && !rec.opts.onPanMove && !rec.opts.onPanEnd)
          continue;
        const slop = rec.opts.panSlop ?? DEFAULT_PAN_SLOP;
        if (adx <= slop && ady <= slop)
          continue;
        const axis = rec.opts.axis ?? "any";
        if (axis === "y" ? ady < adx : axis === "x" ? adx < ady : false) {
          continue;
        }
        rec.flags[t.slot] |= PANNING | TAP_DEAD;
        rec.opts.onPanStart?.(t);
        claim(t, rec);
        break;
      }
    }
    for (const rec of t.owners) {
      if ((rec.flags[t.slot] & (OBSERVING | PANNING)) === (OBSERVING | PANNING)) {
        rec.opts.onPanMove?.(t);
      }
    }
  }
  function finishTrack(t) {
    for (const rec of t.owners) {
      if (rec.pinchA === t.slot || rec.pinchB === t.slot)
        endPinch(rec);
    }
    for (const rec of t.owners) {
      const f = rec.flags[t.slot];
      if (!(f & OBSERVING))
        continue;
      rec.opts.onUp?.(t);
      if (f & PANNING)
        rec.opts.onPanEnd?.(t);
    }
    if (!t.claimedBy) {
      for (const rec of t.owners) {
        const f = rec.flags[t.slot];
        if (!(f & OBSERVING) || f & (TAP_DEAD | LONGPRESS_FIRED | PANNING))
          continue;
        if (!rec.opts.onTap)
          continue;
        rec.opts.onTap(t);
        break;
      }
    }
    releaseTrack(t);
  }
  function __runGestures() {
    const snap = __allTouches();
    if (snap.length === 0 && liveCount === 0)
      return;
    const cancelled = __cancelledTouches();
    for (const t of tracks) {
      if (t.used && cancelled.some((c) => c.surface === t.surface && c.id === t.id)) {
        for (const rec of t.owners)
          if (rec.flags[t.slot] & OBSERVING)
            fireCancel(rec, t);
        releaseTrack(t);
      } else if (t.used && !snap.some((c) => c.surface === t.surface && c.id === t.id))
        finishTrack(t);
      t.present = false;
    }
    for (const c of snap) {
      let found = null;
      for (const t of tracks) {
        if (t.used && t.surface === c.surface && t.id === c.id) {
          found = t;
          break;
        }
      }
      if (found) {
        updateTrack(found, c.x, c.y);
        continue;
      }
      let free = null;
      for (const t of tracks) {
        if (!t.used) {
          free = t;
          break;
        }
      }
      if (free)
        beginTrack(free, c.surface, c.id, c.x, c.y, c.hit);
    }
    for (const t of tracks) {
      if (t.used && !t.present)
        finishTrack(t);
    }
    recognizePinches();
    for (const t of tracks) {
      if (t.used && t.present)
        recognize(t);
    }
  }
  function cancelContactsFor(rec) {
    for (const t of tracks) {
      if (t.used && rec.flags[t.slot] & OBSERVING)
        fireCancel(rec, t);
    }
  }
  function attachGesture(opts) {
    const rec = {
      opts,
      disposed: false,
      flags: new Uint8Array(MAX_TRACKS),
      pairA: -1,
      pairB: -1,
      baseSpan: 0,
      baseCx: 0,
      baseCy: 0,
      prevSpan: 0,
      pinchA: -1,
      pinchB: -1
    };
    recognizers.push(rec);
    return {
      dispose() {
        if (rec.disposed)
          return;
        cancelContactsFor(rec);
        endPinch(rec);
        rec.disposed = true;
        const i = recognizers.lastIndexOf(rec);
        if (i >= 0)
          recognizers.splice(i, 1);
      },
      cancel() {
        if (!rec.disposed)
          cancelContactsFor(rec);
      },
      get panning() {
        for (const t of tracks) {
          if (t.used && rec.flags[t.slot] & PANNING)
            return true;
        }
        return false;
      },
      get pinching() {
        return rec.pinchA >= 0;
      }
    };
  }
  function resetGestures() {
    recognizers.length = 0;
    blockDepth = 0;
    liveCount = 0;
    for (const t of tracks) {
      t.used = false;
      t.present = false;
      t.owners.length = 0;
      t.claimedBy = null;
    }
  }

  // ../../build/pocketjs-upstream/framework/src/gesture.ts
  function createGesture(opts) {
    const handle = attachGesture(opts);
    onCleanup(() => handle.dispose());
    return handle;
  }

  // ../../build/pocketjs-upstream/framework/src/touch-activation.ts
  function installTouchActivation() {
    const install = (surface) => createGesture({
      surface,
      onDown: (c) => {
        const target = touchFocusable(c.x, c.y, c.hit, c.surface);
        if (target)
          setActiveNode(target);
      },
      onTap: (c) => {
        setActiveNode(null);
        const target = touchFocusable(c.x, c.y, c.hit, c.surface);
        if (target)
          pressNode(target);
      },
      onUp: () => setActiveNode(null),
      onCancel: () => setActiveNode(null)
    });
    const primary = install("primary");
    const auxiliary = install("auxiliary");
    return {
      dispose() {
        primary.dispose();
        auxiliary.dispose();
      },
      cancel() {
        primary.cancel();
        auxiliary.cancel();
      },
      get panning() {
        return primary.panning || auxiliary.panning;
      },
      get pinching() {
        return primary.pinching || auxiliary.pinching;
      }
    };
  }

  // ../../build/pocketjs-upstream/contracts/spec/input.ts
  var INPUT_CLASSES = {
    buttons: {
      bit: 1 << 0,
      constant: "BUTTONS",
      capabilities: ["input.buttons"]
    },
    "axis.absolute": {
      bit: 1 << 1,
      constant: "AXIS_ABSOLUTE",
      capabilities: ["input.analog.left", "input.analog.right"]
    },
    "axis.relative": {
      bit: 1 << 2,
      constant: "AXIS_RELATIVE",
      capabilities: ["input.axis.relative"]
    },
    pointer: {
      bit: 1 << 3,
      constant: "POINTER",
      capabilities: ["input.pointer"]
    },
    contact: {
      bit: 1 << 4,
      constant: "CONTACT",
      capabilities: ["input.touch", "input.touch.auxiliary"]
    },
    text: {
      bit: 1 << 5,
      constant: "TEXT",
      capabilities: ["input.text", "input.ime"]
    },
    motion: {
      bit: 1 << 6,
      constant: "MOTION",
      capabilities: ["input.motion.gravity", "input.motion.inertial", "input.motion.geomagnetic"]
    }
  };
  var ABSOLUTE_AXES = {
    LEFT_X: 0,
    LEFT_Y: 1,
    RIGHT_X: 2,
    RIGHT_Y: 3
  };
  var TEXT_EVENT_KINDS = {
    KEY_DOWN: 0,
    KEY_UP: 1,
    COMMIT: 2,
    COMPOSITION: 3,
    COMPOSITION_END: 4
  };
  var constantName = (name) => name.replace(/([a-z0-9])([A-Z])/g, "$1_$2").toUpperCase();
  var INPUT_ABI_CONSTANTS = {
    ...Object.fromEntries(Object.values(INPUT_CLASSES).map((value) => [`POCKET_INPUT_CLASS_${value.constant}`, value.bit])),
    ...Object.fromEntries(Object.entries(BTN).map(([name, value]) => [`POCKET_BUTTON_${name}`, value])),
    POCKET_ANALOG_CENTER: ANALOG_CENTER,
    ...Object.fromEntries(Object.entries(ABSOLUTE_AXES).map(([name, value]) => [`POCKET_AXIS_${name}`, value])),
    ...Object.fromEntries(Object.entries(RelativeAxis).map(([name, value]) => [`POCKET_RELATIVE_AXIS_${constantName(name)}`, value])),
    ...Object.fromEntries(Object.entries(RelativeAxisUnits).map(([name, value]) => [`POCKET_RELATIVE_AXIS_${constantName(name)}`, value])),
    ...Object.fromEntries(Object.entries(TEXT_EVENT_KINDS).map(([name, value]) => [`POCKET_TEXT_${name}`, value])),
    ...Object.fromEntries(Object.entries(MotionLevel).map(([name, value]) => [`POCKET_MOTION_LEVEL_${constantName(name)}`, value])),
    ...Object.fromEntries(Object.entries(MotionQuality).map(([name, value]) => [`POCKET_MOTION_QUALITY_${constantName(name)}`, value])),
    ...Object.fromEntries(Object.entries(MotionReferenceFrame).map(([name, value]) => [`POCKET_MOTION_REFERENCE_${constantName(name)}`, value])),
    ...Object.fromEntries(Object.entries(MOTION_VALUES).map(([name, value]) => [`POCKET_MOTION_VALUE_${constantName(name)}`, value.id]))
  };

  // ../../build/pocketjs-upstream/framework/src/platform.ts
  var features = typeof { "input.buttons": true, "text.glyphs.baked": true } === "object" && { "input.buttons": true, "text.glyphs.baked": true } !== null ? Object.freeze({
    ...{ "input.buttons": true, "text.glyphs.baked": true }
  }) : Object.freeze({});
  var platform = Object.freeze({
    target: "r36s",
    pixelRatio: Number.isInteger(1) ? 1 : 1,
    features
  });
  function hasFeature(feature) {
    return platform.features[feature] === true;
  }

  // ../../build/pocketjs-upstream/framework/src/text-input.ts
  var hasTextInput = hasFeature("input.text");
  var handlers = [];
  var pending4 = [];
  function decodeTextEvent(record) {
    switch (record.kind) {
      case TEXT_EVENT_KINDS.KEY_DOWN:
      case TEXT_EVENT_KINDS.KEY_UP:
        return {
          kind: "key",
          key: record.text ?? "",
          pressed: record.kind === TEXT_EVENT_KINDS.KEY_DOWN,
          repeat: record.repeat === true
        };
      case TEXT_EVENT_KINDS.COMMIT:
        return {
          kind: "commit",
          text: record.text ?? ""
        };
      case TEXT_EVENT_KINDS.COMPOSITION:
        return {
          kind: "composition",
          text: record.text ?? "",
          selectionStart: record.selectionStart ?? 0,
          selectionEnd: record.selectionEnd ?? 0
        };
      case TEXT_EVENT_KINDS.COMPOSITION_END:
        return {
          kind: "compositionEnd"
        };
      default:
        return null;
    }
  }
  function __setTextEvents(records) {
    if (!records || records.length === 0) {
      pending4 = [];
      return;
    }
    const events = [];
    for (const record of records) {
      const event = decodeTextEvent(record);
      if (event)
        events.push(event);
    }
    pending4 = events;
  }
  function __dispatchTextEvents() {
    if (pending4.length === 0)
      return;
    const events = pending4;
    pending4 = [];
    for (const event of events) {
      for (let i = handlers.length - 1;i >= 0; i--) {
        if (handlers[i](event) === true)
          break;
      }
    }
  }
  function __resetTextInput() {
    handlers.length = 0;
    pending4 = [];
  }

  // ../../build/pocketjs-upstream/framework/src/effects.ts
  var nextId = 1;
  var pending5 = new Map;
  var queue = [];
  function traceSink() {
    const s = globalThis.__pocketEffectTrace;
    return typeof s === "function" ? s : null;
  }
  function resetEffects() {
    nextId = 1;
    pending5.clear();
    queue = [];
  }
  function __drainEffects() {
    if (queue.length === 0)
      return;
    const batch = queue;
    queue = [];
    for (const {
      id,
      result
    } of batch) {
      const entry = pending5.get(id);
      if (!entry)
        continue;
      pending5.delete(id);
      traceSink()?.({
        t: "delivery",
        frame: virtualFrame(),
        id,
        kind: entry.kind
      });
      entry.onResult(result);
    }
  }

  // ../../build/pocketjs-upstream/framework/src/services.ts
  var pumps = new Set;
  function runServicePumps() {
    if (pumps.size === 0)
      return;
    for (const pump of pumps)
      pump();
  }

  // ../../build/pocketjs-upstream/framework/src/system-layers.ts
  var layers = [];
  var filter = null;
  var input = {
    buttons: 0,
    touches: undefined,
    hits: undefined,
    surfaces: undefined
  };
  function mountSystemLayers(ops) {
    const disposers = [];
    for (const layer of layers) {
      const dispose = layer(ops);
      if (dispose)
        disposers.push(dispose);
    }
    return () => {
      for (const dispose of disposers.splice(0).reverse())
        dispose();
      filter = null;
    };
  }
  function filterFrameInput(buttons, touches, hits, surfaces) {
    if (filter === null)
      return null;
    input.buttons = buttons;
    input.touches = touches;
    input.hits = hits;
    input.surfaces = surfaces;
    filter(input);
    return input;
  }

  // pocketjs-generated-styles:/home/jostx/Proyectos/Games/r36s/nexus-launcher/build/pocketjs-upstream/framework/src/styles.generated.ts
  var STYLE_IDS = {
    "flex-col w-full h-full px-7 py-5 gap-3 bg-slate-950": 0,
    "flex-row items-center justify-between": 1,
    "flex-col gap-1": 2,
    "text-sm text-cyan-400 font-bold": 3,
    "text-2xl text-white font-bold": 4,
    "text-sm text-slate-400": 5,
    "flex-row gap-3": 6,
    "flex-col flex-1 gap-2 p-3 rounded-lg bg-slate-800 border-slate-700": 7,
    "text-xs text-slate-400 font-bold": 8,
    "flex-row flex-wrap gap-2": 9,
    "px-3 py-2 rounded-md border bg-emerald-600 border-emerald-400": 10,
    "px-3 py-2 rounded-md border bg-slate-900 border-slate-700": 11,
    "text-sm font-bold text-white": 12,
    "text-sm font-bold text-slate-500": 13,
    "flex-row items-center justify-between px-4 py-3 rounded-lg bg-slate-800 border-slate-700": 14,
    "text-xs text-slate-400": 15,
    "text-base text-cyan-300 font-bold": 16,
    "text-base text-white font-bold": 17,
    "text-xs text-slate-500": 18,
    grow: 19,
    "relative flex-col w-full h-full bg-slate-50 overflow-hidden": 20,
    "absolute inset-0 z-50 flex-col items-center justify-center": 21,
    "absolute inset-0 bg-slate-950": 22,
    "flex-col gap-2 w-[328] p-3 rounded-xl shadow-lg bg-white border-slate-200": 23,
    "absolute left-3 right-3 bottom-3 flex-row items-center justify-between px-2 py-1 rounded-lg shadow-md bg-white border-slate-200": 24,
    "flex-row flex-wrap": 25
  };

  // ../../build/pocketjs-upstream/framework/src/index.ts
  if (typeof globalThis.queueMicrotask !== "function") {
    globalThis.queueMicrotask = (fn) => {
      Promise.resolve().then(fn);
    };
  }
  var STYLES_KEY = "ui:styles";
  var FONT_PREFIX = "ui:font.";
  var IMG_PREFIX = "ui:img.";
  var SPRITE_PREFIX = "ui:sprite.";
  function globalOps() {
    return globalThis.ui;
  }
  function uploadPakImages(ops) {
    if (ops.__textures)
      return;
    for (const key of entries(IMG_PREFIX)) {
      const blob = get(key);
      let handle;
      if (ops.uploadImgEntry) {
        handle = ops.uploadImgEntry(blob);
      } else {
        const dv = new DataView(blob.buffer, blob.byteOffset, blob.byteLength);
        handle = ops.uploadTexture(blob.subarray(8), dv.getUint16(0, true), dv.getUint16(2, true), blob[4]);
      }
      if (handle >= 0)
        registerTexture(key.slice(IMG_PREFIX.length), handle);
    }
  }
  function uploadPakSprites(ops) {
    if (ops.__sprites)
      return;
    for (const key of entries(SPRITE_PREFIX)) {
      const blob = get(key);
      const dv = new DataView(blob.buffer, blob.byteOffset, blob.byteLength);
      const w = dv.getUint16(0, true);
      const h = dv.getUint16(2, true);
      const psm = blob[4];
      const frames = dv.getUint16(6, true);
      const cols = dv.getUint16(8, true);
      const step = dv.getUint16(10, true);
      const handle = ops.uploadTexture(blob.subarray(16), w, h, psm);
      if (handle >= 0) {
        registerSprite(key.slice(SPRITE_PREFIX.length), {
          handle,
          frames,
          cols,
          step
        });
      }
    }
  }
  function createLayer(style) {
    const layer = createElement2("view");
    setProp(layer, "style", style, undefined);
    return layer;
  }
  var appLayer = null;
  var overlayLayer = null;
  function resizeViewport(w, h) {
    if (!appLayer || !overlayLayer)
      return;
    setProp(appLayer, "style", {
      width: w,
      height: h,
      overflow: ENUMS.Overflow.Hidden
    }, undefined);
    setProp(overlayLayer, "style", {
      width: w,
      height: h,
      posType: ENUMS.PosType.Absolute,
      insetT: 0,
      insetR: 0,
      insetB: 0,
      insetL: 0,
      zIndex: 1000
    }, undefined);
    const ops = getOps();
    ops.__viewport = {
      w,
      h
    };
  }
  var runDeferredGestures = (defer) => withDeferredPress(defer, __runGestures);
  function render2(code, opts = {}) {
    const host = detectHost(opts.ops);
    installHost(host);
    setStyleResolver(resolveStyle);
    if (opts.styles)
      registerStyles(opts.styles);
    const nativeTextureTable = host.kind === "native" ? host.ops.__textures : undefined;
    if (host.kind === "native") {
      if (nativeTextureTable) {
        for (const key in nativeTextureTable) {
          registerTexture(key, nativeTextureTable[key]);
        }
      }
      const spr = host.ops.__sprites;
      if (spr) {
        for (const key in spr)
          registerSprite(key, spr[key]);
      }
    }
    if (host.kind === "injected" || nativeTextureTable === undefined) {
      if (opts.pak)
        loadPack(opts.pak);
      if (hasPack()) {
        for (const key of entries()) {
          if (key === STYLES_KEY) {
            host.ops.loadStyles?.(get(key));
          } else if (key.startsWith(FONT_PREFIX)) {
            host.ops.loadFontAtlas?.(get(key));
          }
        }
      }
    }
    const auxiliary = mountAuxiliarySurface(host.ops);
    const viewport = hostViewport(host.ops);
    const layerW = viewport?.w ?? SCREEN_W;
    const layerH = viewport?.h ?? SCREEN_H;
    const appRoot = createLayer({
      width: layerW,
      height: layerH,
      overflow: ENUMS.Overflow.Hidden
    });
    const overlayRoot = createLayer({
      width: layerW,
      height: layerH,
      posType: ENUMS.PosType.Absolute,
      insetT: 0,
      insetR: 0,
      insetB: 0,
      insetL: 0,
      zIndex: 1000,
      hitPass: 1
    });
    insertNode(rootMirror, appRoot);
    insertNode(rootMirror, overlayRoot);
    setOverlayRoot(overlayRoot);
    appLayer = appRoot;
    overlayLayer = overlayRoot;
    setInputRoot(appRoot);
    setHitRoot(rootMirror);
    setAuxiliaryHitRoot(auxiliary?.native ?? null);
    resetFrameHooks();
    resetGestures();
    installTouchActivation();
    resetClock();
    resetEffects();
    initDevtools(host.ops);
    installFrameHandler(wrapFrameHandler((buttons, analog, touches, hits, touchSurfaces, rightAnalog, axisDeltas, motion, text) => {
      __advanceClock();
      const filtered = filterFrameInput(buttons, touches, hits, touchSurfaces);
      if (filtered !== null) {
        buttons = filtered.buttons;
        touches = filtered.touches;
        hits = filtered.hits;
        touchSurfaces = filtered.surfaces;
      }
      __setAnalog(analog, rightAnalog);
      __setTouches(touches, hits, touchSurfaces);
      __setTextEvents(text);
      runServicePumps();
      __drainEffects();
      __dispatchTextEvents();
      runFrameHooks(buttons, axisDeltas, (defer) => handleFrame(buttons, defer), runDeferredGestures, motion);
      runSweep();
    }));
    const dispose = render(code, appRoot);
    const disposeSystemLayers = mountSystemLayers(host.ops);
    flushLifecycleHooks();
    const removeResizeViewportHook = installResizeViewportHook(resizeViewport);
    return () => {
      removeResizeViewportHook();
      disposeSystemLayers();
      __resetTouches();
      __resetTextInput();
      resetGestures();
      dispose();
      disposeRootModelRegions();
      flushUnmountedHooks();
      setInputRoot(null);
      setHitRoot(null);
      setAuxiliaryHitRoot(null);
      setOverlayRoot(null);
      appLayer = null;
      overlayLayer = null;
      for (const child of rootMirror.children.splice(0)) {
        child.parent = null;
        host.ops.destroyNode(child.id);
      }
      unmountAuxiliarySurface(host.ops);
      runSweep();
    };
  }
  function mount(code, opts = {}) {
    const ops = opts.ops ?? globalOps();
    if (!ops) {
      throw new Error("PocketJS: mount() requires globalThis.ui or opts.ops");
    }
    if (opts.pak)
      loadPack(opts.pak);
    uploadPakImages(ops);
    uploadPakSprites(ops);
    const dispose = render2(code, {
      ops,
      styles: opts.styles ?? STYLE_IDS,
      pak: opts.pak
    });
    return dispose;
  }

  // ../../build/pocketjs-upstream/framework/src/resource-boundary.ts
  function ResourceBoundary(props) {
    const state = createMemo(props.state);
    const status = createMemo(() => state().status);
    const error = createMemo(() => {
      const value = state();
      return value.status === "error" ? value.error : undefined;
    });
    return createMemo(() => {
      const phase = status();
      const reason = phase === "error" ? error() : undefined;
      if (phase !== "ready")
        return phase === "error" && props.errorFallback ? props.errorFallback(reason) : props.fallback();
      return untrack(() => {
        return props.children(() => {
          const current = state();
          if (current.status !== "ready")
            throw new Error("Resource read outside its ready subtree");
          return current.value;
        });
      });
    });
  }

  // ../../build/pocketjs-upstream/framework/src/resource-state.ts
  var PENDING2 = Object.freeze({
    status: "pending"
  });
  var pending6 = () => PENDING2;

  // ../../build/pocketjs-upstream/framework/src/primitives.ts
  function callRef(ref, node) {
    if (!ref)
      return;
    if (typeof ref === "function")
      ref(node);
    else if ("current" in ref)
      ref.current = node;
  }
  function primitive(tag, props) {
    const el = createElement2(tag);
    spread(el, props, false);
    callRef(props.nodeRef, el);
    return el;
  }
  function View(props) {
    return primitive("view", props);
  }
  function Text(props) {
    if ("resource" in props) {
      const [state, setState] = createSignal(pending6());
      createRenderEffect(() => {
        const resource = props.resource;
        const update = () => setState(() => resource?.state() ?? pending6());
        untrack(update);
        const unsubscribe = resource?.subscribe(update);
        onCleanup(() => unsubscribe?.());
      });
      return ResourceBoundary({
        state,
        fallback: () => props.fallback?.() ?? null,
        errorFallback: (error) => props.errorFallback?.(error) ?? props.fallback?.() ?? null,
        children: (value) => primitive("text", {
          get class() {
            return props.class;
          },
          get style() {
            return {
              ...props.style,
              fontSlot: value().slot
            };
          },
          get debugName() {
            return props.debugName;
          },
          get ref() {
            return props.ref;
          },
          get nodeRef() {
            return props.nodeRef;
          },
          get children() {
            return value().text;
          }
        })
      });
    }
    return primitive("text", props);
  }
  // ../../build/pocketjs-upstream/framework/src/hot.ts
  var lastText = new WeakMap;
  var lastProp = new WeakMap;

  // ../../build/pocketjs-upstream/framework/src/tiles.ts
  var parsed = new Map;

  // ../../build/pocketjs-upstream/framework/src/deepzoom.ts
  var TICK_SCALE = 60 / TICKS_PER_SECOND;
  var perTick = (at60) => TICK_SCALE === 1 ? at60 : at60 ** TICK_SCALE;
  var PAN_SPEED = 7 * TICK_SCALE;
  var DPAD_SPEED = 5 * TICK_SCALE;
  var ZOOM_STEP = perTick(1.035);
  var VEL_APPROACH = TICK_SCALE === 1 ? 0.35 : 1 - perTick(1 - 0.35);
  var VEL_DECAY = perTick(0.88);
  // main.tsx
  var FN = 1 << 16;
  var PLUS = 1 << 17;
  var MINUS = 1 << 18;
  var UNKNOWN = 1 << 19;
  var controls = [{
    name: "UP",
    mask: BTN.UP,
    group: "D-PAD"
  }, {
    name: "DOWN",
    mask: BTN.DOWN,
    group: "D-PAD"
  }, {
    name: "LEFT",
    mask: BTN.LEFT,
    group: "D-PAD"
  }, {
    name: "RIGHT",
    mask: BTN.RIGHT,
    group: "D-PAD"
  }, {
    name: "X",
    mask: BTN.SQUARE,
    group: "FACE"
  }, {
    name: "Y",
    mask: BTN.TRIANGLE,
    group: "FACE"
  }, {
    name: "A",
    mask: BTN.CIRCLE,
    group: "FACE"
  }, {
    name: "B",
    mask: BTN.CROSS,
    group: "FACE"
  }, {
    name: "L1",
    mask: BTN.LTRIGGER,
    group: "SHOULDERS"
  }, {
    name: "R1",
    mask: BTN.RTRIGGER,
    group: "SHOULDERS"
  }, {
    name: "L2",
    mask: BTN.ZL,
    group: "SHOULDERS"
  }, {
    name: "R2",
    mask: BTN.ZR,
    group: "SHOULDERS"
  }, {
    name: "SELECT",
    mask: BTN.SELECT,
    group: "SYSTEM"
  }, {
    name: "START",
    mask: BTN.START,
    group: "SYSTEM"
  }, {
    name: "FN",
    mask: FN,
    group: "SYSTEM"
  }, {
    name: "PLUS",
    mask: PLUS,
    group: "SYSTEM"
  }, {
    name: "MINUS",
    mask: MINUS,
    group: "SYSTEM"
  }, {
    name: "DESCONOCIDO",
    mask: UNKNOWN,
    group: "SYSTEM"
  }];
  function GamepadTest() {
    const [held, setHeld] = createSignal(0);
    const [lastPressed, setLastPressed] = createSignal("—");
    const [pressCount, setPressCount] = createSignal(0);
    let previous = 0;
    onFrame((buttons) => {
      const pressed = buttons & ~previous;
      const keyCode = buttons >>> 20 & 1023;
      const previousKeyCode = previous >>> 20 & 1023;
      if (buttons !== previous)
        setHeld(buttons);
      if (pressed !== 0 || keyCode !== previousKeyCode) {
        const names = controls.filter((control) => (buttons & control.mask) !== 0).map((control) => control.name);
        const keyId = keyCode > 0 ? `KEY ${keyCode} (0x${keyCode.toString(16).padStart(3, "0")})` : "KEY —";
        setLastPressed(`${names.join(" + ") || "DESCONOCIDO"} · ${keyId}`);
        if (pressed !== 0)
          setPressCount((count) => count + Math.max(names.length, 1));
      }
      previous = buttons;
    });
    const groups = ["D-PAD", "FACE", "SHOULDERS", "SYSTEM"];
    return createComponent2(View, {
      class: "flex-col w-full h-full px-7 py-5 gap-3 bg-slate-950",
      get children() {
        return [createComponent2(View, {
          class: "flex-row items-center justify-between",
          get children() {
            return [createComponent2(View, {
              class: "flex-col gap-1",
              get children() {
                return [createComponent2(Text, {
                  class: "text-sm text-cyan-400 font-bold",
                  children: "NEXUS · PRUEBA DE MANDO R36S"
                }), createComponent2(Text, {
                  class: "text-2xl text-white font-bold",
                  children: "Gamepad tester"
                })];
              }
            }), createComponent2(Text, {
              class: "text-sm text-slate-400",
              children: "640 × 480"
            })];
          }
        }), createComponent2(View, {
          class: "flex-row gap-3",
          get children() {
            return groups.map((group) => createComponent2(View, {
              class: "flex-col flex-1 gap-2 p-3 rounded-lg bg-slate-800 border-slate-700",
              get children() {
                return [createComponent2(Text, {
                  class: "text-xs text-slate-400 font-bold",
                  children: group
                }), createComponent2(View, {
                  class: "flex-row flex-wrap gap-2",
                  get children() {
                    return controls.filter((control) => control.group === group).map((control) => createComponent2(View, {
                      get ["class"]() {
                        return held() & control.mask ? "px-3 py-2 rounded-md border bg-emerald-600 border-emerald-400" : "px-3 py-2 rounded-md border bg-slate-900 border-slate-700";
                      },
                      get children() {
                        return createComponent2(Text, {
                          get ["class"]() {
                            return held() & control.mask ? "text-sm font-bold text-white" : "text-sm font-bold text-slate-500";
                          },
                          get children() {
                            return [memo2(() => control.name), memo2(() => held() & control.mask ? " · DOWN" : "")];
                          }
                        });
                      }
                    }));
                  }
                })];
              }
            }));
          }
        }), createComponent2(View, {
          class: "flex-row items-center justify-between px-4 py-3 rounded-lg bg-slate-800 border-slate-700",
          get children() {
            return [createComponent2(View, {
              class: "flex-col gap-1",
              get children() {
                return [createComponent2(Text, {
                  class: "text-xs text-slate-400",
                  children: "ÚLTIMO BOTÓN"
                }), createComponent2(Text, {
                  class: "text-base text-cyan-300 font-bold",
                  get children() {
                    return lastPressed();
                  }
                })];
              }
            }), createComponent2(View, {
              class: "flex-col gap-1",
              get children() {
                return [createComponent2(Text, {
                  class: "text-xs text-slate-400",
                  children: "PULSACIONES"
                }), createComponent2(Text, {
                  class: "text-base text-white font-bold",
                  get children() {
                    return pressCount();
                  }
                })];
              }
            }), createComponent2(Text, {
              class: "text-xs text-slate-500",
              children: "Mantén y suelta cada control"
            })];
          }
        }), createComponent2(Text, {
          class: "text-xs text-slate-500",
          children: "ÚLTIMO BOTÓN incluye su código Linux KEY (decimal y hexadecimal) · sticks analógicos no incluidos"
        })];
      }
    });
  }
  mount(() => createComponent2(GamepadTest, {}));
})();
