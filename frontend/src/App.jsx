import { useState } from 'react'
import './App.css'

const API = 'http://localhost:5000/api'

const SAMPLES = [
  {
    name: 'Basic + Constant Folding',
    code: `x = 3 + 5 * 2;
y = x - 4;
z = 100 / 5 + y;`
  },
  {
    name: 'Parentheses & Unary',
    code: `a = -(10 - 4) * 2;
b = a + 5;`
  },
  {
    name: 'Semantic Error',
    code: `x = 10;
y = x + undeclared_var;`
  },
  {
    name: 'Syntax Error',
    code: `x = 3 + * 5;`
  },
  {
    name: 'Lexical Error',
    code: `x = 3 $ 5;`
  }
]

const PHASE_HELP = {
  tokens: {
    title: 'What is Lexical Analysis?',
    text: 'The lexer reads your source code character by character and breaks it into meaningful pieces called Tokens (numbers, names, operators, etc.). Invalid characters are reported with exact line and column.'
  },
  ast: {
    title: 'What is Syntax Analysis?',
    text: 'The parser checks whether the tokens follow the grammar rules and builds an Abstract Syntax Tree (Parse Tree). This tree shows the structure of your program.'
  },
  semantic: {
    title: 'What is Semantic Analysis + Optimization?',
    text: 'We build a Symbol Table for variables, detect undeclared variables, generate Three Address Code (TAC), and apply Constant Folding — computing constant expressions at compile time to reduce instructions.'
  },
  summary: {
    title: 'Compilation Summary',
    text: 'Overall result of all phases. Use the sample codes to see how different errors and optimizations appear.'
  }
}

// Recursive Tree Node Component
const TreeNode = ({ node }) => {
  if (!node) return null;

  return (
    <div className="tree-node-container">
      <div className={`tree-node ${node.type.toLowerCase()}`}>
        <span className="node-type">{node.type}</span>
        {node.value && node.value !== 'program' && node.value !== 'params' && (
          <span className="node-value">{node.value}</span>
        )}
      </div>
      {node.children && node.children.length > 0 && (
        <div className="tree-children">
          {node.children.map((child, i) => (
            <div key={i} className="tree-child">
              <TreeNode node={child} />
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

function App() {
  const [source, setSource] = useState(SAMPLES[0].code)
  const [result, setResult] = useState(null)
  const [loading, setLoading] = useState(false)
  const [activeTab, setActiveTab] = useState('tokens')
  const [activeICG, setActiveICG] = useState('tac')
  const [activeAST, setActiveAST] = useState('ast') // 'ast' or 'pt'
  const [viewType, setViewType] = useState('visual') // 'visual' or 'text'
  const [error, setError] = useState(null)
  const [isSidebarOpen, setIsSidebarOpen] = useState(false)

  // Logic to convert TAC to Triples
  const getTriples = (tacList) => {
    if (!tacList || !Array.isArray(tacList)) return []
    const tempToIndex = {}

    return tacList.map((instr, idx) => {
      // Handle both Object and String format for backward compatibility
      let result = '', op = '', arg1 = '', arg2 = '-', hasArg2 = false

      if (typeof instr === 'string') {
        const match = instr.match(/^(.+?)\s*=\s*(.+?)(?:\s+([\+\-\*\/])\s+(.+))?$/)
        if (match) {
          result = match[1].trim(); arg1 = match[2].trim();
          op = match[3] || '='; arg2 = match[4] || '-'; hasArg2 = !!match[3]
        } else {
          const s = instr.split('='); result = s[0]?.trim(); arg1 = s[1]?.trim(); op = '='
        }
      } else {
        result = instr.result || ''; op = instr.op || '=';
        arg1 = instr.arg1 || ''; arg2 = instr.arg2 || '-'; hasArg2 = instr.has_arg2
      }

      if (result && result.startsWith('t')) {
        tempToIndex[result] = `(${idx})`
      }

      const formatValue = (val) => tempToIndex[val] || val

      return {
        idx,
        op: op,
        arg1: formatValue(arg1),
        arg2: hasArg2 ? formatValue(arg2) : '-'
      }
    })
  }

  const compile = async () => {
    setLoading(true)
    setError(null)
    try {
      const res = await fetch(`${API}/compile`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ source })
      })
      const data = await res.json()
      if (!res.ok) {
        setError(data.error || 'Compilation failed')
        setResult(null)
      } else {
        setResult(data)
        setActiveTab('tokens')
      }
    } catch (e) {
      setError('Cannot connect to backend. Make sure Flask is running on port 5000.')
      setResult(null)
    } finally {
      setLoading(false)
    }
  }

  const loadSample = (code) => {
    setSource(code)
    setResult(null)
    setError(null)
  }

  const help = PHASE_HELP[activeTab]

  return (
    <div className={`app ${isSidebarOpen ? 'sidebar-open' : ''}`}>
      {/* Mobile Overlay */}
      <div className="sidebar-overlay" onClick={() => setIsSidebarOpen(false)}></div>

      {/* Premium Loading Overlay */}
      {loading && (
        <div className="loader-overlay">
          <div className="loader-content">
            <div className="spinner">
              <div className="inner-spinner"></div>
            </div>
            <div className="loader-text">
              <h3>Analyzing Code...</h3>
              <p>Lexing • Parsing • Optimizing</p>
            </div>
          </div>
        </div>
      )}

      <aside className={`sidebar ${isSidebarOpen ? 'open' : ''}`}>
        <div className="logo">
          <span className="logo-icon">⚙️</span>
          <div>
            <h1>EduCompile</h1>
            <p>Learn Compiler Phases</p>
          </div>
          <button className="mobile-close" onClick={() => setIsSidebarOpen(false)}>✕</button>
        </div>

        <nav className="nav">
          <button className={activeTab === 'tokens' ? 'active' : ''} onClick={() => setActiveTab('tokens')} disabled={!result}>
            1. Lexical Analysis
          </button>
          <button className={activeTab === 'ast' ? 'active' : ''} onClick={() => setActiveTab('ast')} disabled={!result}>
            2. Syntax Analysis
          </button>
          <button className={activeTab === 'semantic' ? 'active' : ''} onClick={() => setActiveTab('semantic')} disabled={!result}>
            3. Semantic + Optimization
          </button>
          <button className={activeTab === 'summary' ? 'active' : ''} onClick={() => setActiveTab('summary')} disabled={!result}>
            Summary & Errors
          </button>
          <button className={activeTab === 'roadmap' ? 'active' : ''} onClick={() => setActiveTab('roadmap')} disabled={!result}>
            Full Compiler Roadmap
          </button>
        </nav>

        <div className="samples">
          <h3>Try These Samples</h3>
          {SAMPLES.map((s, i) => (
            <button key={i} className="sample-btn" onClick={() => loadSample(s.code)}>
              {s.name}
            </button>
          ))}
        </div>

        <div className="learning-box">
          <h3>Why this tool?</h3>
          <ul>
            <li>See every compiler phase visually</li>
            <li>Errors show exact Line & Column</li>
            <li>Before vs After Optimization</li>
            <li>Perfect for class demos</li>
          </ul>
        </div>

        <div className="tech-stack">
          <small>Core: C | Backend: Flask | Frontend: React</small>
        </div>
      </aside>

      <main className="main">
        <header className="topbar">
          <div className="topbar-left">
            <button className="menu-toggle" onClick={() => setIsSidebarOpen(true)}>
              ☰
            </button>
            <h2>Interactive Compiler Phase Visualizer</h2>
          </div>
        </header>

        <div className="editor-panel">
          <label>Source Code (MiniExpr Language)</label>
          <textarea
            value={source}
            onChange={(e) => setSource(e.target.value)}
            spellCheck={false}
            placeholder="// Write code here and click Compile..."
          />
          <div className="editor-footer">
            <button className="compile-btn" onClick={compile} disabled={loading}>
              {loading ? 'Compiling…' : '▶ Compile'}
            </button>
          </div>
        </div>

        {error && (
          <div className="alert error">
            <strong>Error:</strong> {error}
          </div>
        )}

        {result && (
          <div className="results">
            <div className="badges">
              <span className={`badge ${result.summary?.lexical_errors === 0 ? 'ok' : 'err'}`}>
                Lexical: {result.summary?.lexical_errors ?? 0} errors
              </span>
              <span className={`badge ${result.summary?.syntax_errors === 0 ? 'ok' : 'err'}`}>
                Syntax: {result.summary?.syntax_errors ?? 0} errors
              </span>
              <span className={`badge ${result.summary?.semantic_errors === 0 ? 'ok' : 'err'}`}>
                Semantic: {result.summary?.semantic_errors ?? 0} errors
              </span>
              <span className="badge info">
                TAC: {result.summary?.tac_before_count ?? 0} → {result.summary?.tac_count ?? 0} (optimized)
              </span>
            </div>

            {/* Educational help box */}
            {help && (
              <div className="help-box">
                <strong>{help.title}</strong>
                <p>{help.text}</p>
              </div>
            )}

            <div className="tab-content">
              {activeTab === 'tokens' && (
                <div>
                  <h3>Token Stream</h3>
                  {result.lexical_errors?.length > 0 && (
                    <div className="alert error">
                      {result.lexical_errors.map((e, i) => <div key={i}>🔴 {e}</div>)}
                    </div>
                  )}
                  <table className="token-table">
                    <thead>
                      <tr><th>Line</th><th>Column</th><th>Lexeme</th><th>Token Type</th></tr>
                    </thead>
                    <tbody>
                      {result.tokens?.map((t, i) => (
                        <tr key={i} className={t.type === 'INVALID' ? 'invalid' : ''}>
                          <td>{t.line}</td>
                          <td>{t.column}</td>
                          <td><code>{t.value}</code></td>
                          <td><code className={`tok-${t.type}`}>{t.type}</code></td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}

              {activeTab === 'ast' && (
                <div>
                  <div className="tab-header">
                    <div className="icg-nav" style={{ marginBottom: 0, border: 'none' }}>
                      <button className={activeAST === 'pt' ? 'active' : ''} onClick={() => setActiveAST('pt')}>Parse Tree (Concrete)</button>
                      <button className={activeAST === 'ast' ? 'active' : ''} onClick={() => setActiveAST('ast')}>AST (Abstract)</button>
                    </div>
                    <div className="view-toggle">
                      <button className={viewType === 'visual' ? 'active' : ''} onClick={() => setViewType('visual')}>Graphical</button>
                      <button className={viewType === 'text' ? 'active' : ''} onClick={() => setViewType('text')}>Text Outline</button>
                    </div>
                  </div>

                  <div className="help-box" style={{ background: 'rgba(167, 139, 250, 0.05)', borderColor: 'rgba(167, 139, 250, 0.2)' }}>
                    {activeAST === 'pt' ? (
                      <>
                        <strong>Parse Tree (CST):</strong>
                        <p>Shows the complete grammatical structure including non-terminals like <code>Expression</code>, <code>Factor</code>, etc. It represents <em>how</em> the parser recognized the code.</p>
                      </>
                    ) : (
                      <>
                        <strong>Abstract Syntax Tree (AST):</strong>
                        <p>A simplified tree that only contains essential info (operators and operands). It represents the <em>meaning</em> of the code by removing redundant grammar rules.</p>
                      </>
                    )}
                  </div>

                  {result.syntax_errors?.length > 0 && (
                    <div className="alert error">
                      {result.syntax_errors.map((e, i) => <div key={i}>🔴 {e}</div>)}
                    </div>
                  )}

                  {viewType === 'visual' ? (
                    <div className="visual-tree-container">
                      {activeAST === 'pt' ? (
                        result.pt_json ? <TreeNode node={result.pt_json} /> : <p className="muted">No Parse Tree data available.</p>
                      ) : (
                        result.ast_json ? <TreeNode node={result.ast_json} /> : <p className="muted">No AST data available.</p>
                      )}
                    </div>
                  ) : (
                    <pre className="ast-view">{result.ast || '(empty)'}</pre>
                  )}
                </div>
              )}

              {activeTab === 'semantic' && (
                <div>
                  <div className="icg-nav">
                    <button className={activeICG === 'tac' ? 'active' : ''} onClick={() => setActiveICG('tac')}>Standard TAC</button>
                    <button className={activeICG === 'quad' ? 'active' : ''} onClick={() => setActiveICG('quad')}>Quadruples</button>
                    <button className={activeICG === 'triple' ? 'active' : ''} onClick={() => setActiveICG('triple')}>Triples</button>
                    <button className={activeICG === 'indirect' ? 'active' : ''} onClick={() => setActiveICG('indirect')}>Indirect Triples</button>
                    <button className={activeICG === 'dag' ? 'active' : ''} onClick={() => setActiveICG('dag')}>DAG Concept</button>
                  </div>

                  {activeICG === 'tac' && (
                    <>
                      <h3>Symbol Table</h3>
                      <table className="token-table" style={{ marginBottom: '1.5rem' }}>
                        <thead>
                          <tr><th>Name</th><th>Type</th><th>Used</th><th>Value</th></tr>
                        </thead>
                        <tbody>
                          {result.symbol_table?.length ? result.symbol_table.map((s, i) => (
                            <tr key={i}>
                              <td><code>{s.name}</code></td>
                              <td>{s.type}</td>
                              <td>{s.used ? '✓' : '—'}</td>
                              <td>{s.value !== null ? s.value : '—'}</td>
                            </tr>
                          )) : (
                            <tr><td colSpan="4">No variables</td></tr>
                          )}
                        </tbody>
                      </table>

                      <div className="compare-grid">
                        <div className="compare-card before">
                          <h4>❌ Before Optimization</h4>
                          <p className="compare-sub">TAC without Constant Folding</p>
                          <pre className="tac-view">
                            {result.tac_before?.length
                              ? result.tac_before.map((t, i) => {
                                  if (typeof t === 'string') return `${String(i).padStart(2)}.  ${t}`;
                                  const line = t.has_op && t.has_arg2 ? `${t.result} = ${t.arg1} ${t.op} ${t.arg2}` :
                                               t.has_op ? `${t.result} = ${t.op}${t.arg1}` : `${t.result} = ${t.arg1}`;
                                  return `${String(i).padStart(2)}.  ${line}`;
                                }).join('\n')
                              : '(empty)'}
                          </pre>
                          <div className="count-badge">{result.summary?.tac_before_count ?? 0} instructions</div>
                        </div>

                        <div className="compare-card after">
                          <h4>✅ After Constant Folding</h4>
                          <p className="compare-sub">Constants computed at compile time</p>
                          <pre className="tac-view">
                            {result.tac?.length
                              ? result.tac.map((t, i) => {
                                  if (typeof t === 'string') return `${String(i).padStart(2)}.  ${t}`;
                                  const line = t.has_op && t.has_arg2 ? `${t.result} = ${t.arg1} ${t.op} ${t.arg2}` :
                                               t.has_op ? `${t.result} = ${t.op}${t.arg1}` : `${t.result} = ${t.arg1}`;
                                  return `${String(i).padStart(2)}.  ${line}`;
                                }).join('\n')
                              : '(empty – fully folded)'}
                          </pre>
                          <div className="count-badge ok">{result.summary?.tac_count ?? 0} instructions</div>
                        </div>
                      </div>
                    </>
                  )}

                  {activeICG === 'quad' && (
                    <div>
                      <h3>Quadruples (op, arg1, arg2, result)</h3>
                      <table className="token-table">
                        <thead>
                          <tr><th>#</th><th>op</th><th>arg1</th><th>arg2</th><th>result</th></tr>
                        </thead>
                        <tbody>
                          {result.tac?.map((t, i) => {
                            let res = '', op = '=', a1 = '', a2 = '-'
                            if (typeof t === 'string') {
                              const match = t.match(/^(.+?)\s*=\s*(.+?)(?:\s+([\+\-\*\/])\s+(.+))?$/)
                              if (match) {
                                res = match[1].trim(); a1 = match[2].trim();
                                op = match[3] || '='; a2 = match[4] || '-'
                              } else {
                                const s = t.split('='); res = s[0]?.trim(); a1 = s[1]?.trim()
                              }
                            } else {
                              res = t.result; a1 = t.arg1; op = t.op || '='; a2 = t.has_arg2 ? t.arg2 : '-'
                            }
                            return (
                              <tr key={i}>
                                <td>{i}</td>
                                <td><code className="tok-OPERATOR">{op}</code></td>
                                <td>{a1}</td>
                                <td>{a2}</td>
                                <td><code className="tok-IDENTIFIER">{res}</code></td>
                              </tr>
                            )
                          })}
                        </tbody>
                      </table>
                    </div>
                  )}

                  {activeICG === 'triple' && (
                    <div>
                      <h3>Triples (op, arg1, arg2)</h3>
                      <p className="compare-sub">Note: References are indices of previous instructions.</p>
                      <table className="token-table">
                        <thead>
                          <tr><th>#</th><th>op</th><th>arg1</th><th>arg2</th></tr>
                        </thead>
                        <tbody>
                          {getTriples(result.tac).map((t, i) => (
                            <tr key={i}>
                              <td>{i}</td>
                              <td><code className="tok-OPERATOR">{t.op}</code></td>
                              <td>{t.arg1}</td>
                              <td>{t.arg2}</td>
                            </tr>
                          ))}
                        </tbody>
                      </table>
                    </div>
                  )}

                  {activeICG === 'indirect' && (
                    <div className="compare-grid" style={{ gridTemplateColumns: '1fr 3fr' }}>
                       <div>
                        <h3>Pointers</h3>
                        <table className="token-table">
                          <thead><tr><th>#</th><th>Stmt</th></tr></thead>
                          <tbody>
                            {result.tac?.map((_, i) => (
                              <tr key={i}><td>{i}</td><td>{(i + 10).toString()}</td></tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                      <div>
                        <h3>Triples</h3>
                        <table className="token-table">
                          <thead><tr><th>#</th><th>op</th><th>arg1</th><th>arg2</th></tr></thead>
                          <tbody>
                            {getTriples(result.tac).map((t, i) => (
                              <tr key={i}>
                                <td>{(i + 10).toString()}</td>
                                <td><code className="tok-OPERATOR">{t.op}</code></td>
                                <td>{t.arg1}</td>
                                <td>{t.arg2}</td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                    </div>
                  )}

                  {activeICG === 'dag' && (
                    <div className="dag-section">
                      <h3>Directed Acyclic Graph (DAG)</h3>
                      <p className="compare-sub">DAG is used for Common Sub-expression Elimination (CSE).</p>

                      <div className="help-box" style={{ background: 'rgba(34, 197, 94, 0.05)', borderColor: 'rgba(34, 197, 94, 0.2)' }}>
                        <strong>How DAG works:</strong>
                        <p>DAG represents expressions where common sub-expressions (like <code>b * c</code> in your image) are merged into a single node. This saves memory and prevents redundant calculations.</p>
                      </div>

                      <div className="dag-comparison">
                        <div className="dag-card">
                           <h4>Syntax Tree</h4>
                           <pre className="ast-view" style={{ fontSize: '0.7rem' }}>
{`    [-]
   /   \\
 [+]    [/]
 / \\    / \\
a  [*] d  [*]
   / \\    / \\
  b   c  b   c`}
                           </pre>
                           <p className="compare-sub">Redundant 'b*c' nodes</p>
                        </div>
                        <div className="arrow-icon">➡</div>
                        <div className="dag-card primary">
                           <h4>DAG (Optimized)</h4>
                           <pre className="ast-view" style={{ fontSize: '0.7rem', color: '#4ade80' }}>
{`    [-]
   /   \\
 [+]    [/]
 / \\    / |
a  [*] -- |
   / \\   /
  b   c`}
                           </pre>
                           <p className="compare-sub">Shared 'b*c' node</p>
                        </div>
                      </div>
                    </div>
                  )}

                  {result.semantic_errors?.length > 0 && (
                    <div className="alert error" style={{ marginTop: '1rem' }}>
                      {result.semantic_errors.map((e, i) => <div key={i}>🔴 {e}</div>)}
                    </div>
                  )}
                </div>
              )}

              {activeTab === 'roadmap' && (
                <div className="roadmap-container">
                  <div className="roadmap-header">
                    <h3>Compiler Design - Full Phase Roadmap</h3>
                    <p>Observe how your code transforms through every stage of the compiler with full analysis.</p>
                  </div>

                  <div className="roadmap-grid">
                    {/* Phase 1: Lexical */}
                    <div className="roadmap-phase">
                      <div className="phase-marker">1</div>
                      <div className="phase-content">
                        <h4>Lexical Analysis (Scanning)</h4>
                        <p className="phase-desc">Scans source code and converts characters into meaningful <strong>Tokens</strong>.</p>
                        <div className="phase-data tokens-mini">
                          {result.tokens?.slice(0, 12).map((t, i) => (
                            <span key={i} className={`mini-token tok-${t.type}`}>{t.value}</span>
                          ))}
                          {result.tokens?.length > 12 && <span>...</span>}
                        </div>
                        <div className="analysis-box">
                           <strong>Phase Analysis:</strong>
                           <ul>
                             <li>Successfully identified <strong>{result.summary?.token_count}</strong> tokens.</li>
                             <li>Categorized elements into Keywords, Identifiers, Operators, and Delimiters.</li>
                             <li>Eliminated whitespace and comments from the processing stream.</li>
                           </ul>
                        </div>
                        <div className="phase-io">Input: Source Code ➜ Output: Token Stream</div>
                      </div>
                    </div>

                    <div className="roadmap-arrow">↓</div>

                    {/* Phase 2: Syntax */}
                    <div className="roadmap-phase">
                      <div className="phase-marker">2</div>
                      <div className="phase-content">
                        <h4>Syntax Analysis (Parsing)</h4>
                        <p className="phase-desc">Checks token arrangements against grammar rules to build an <strong>AST</strong>.</p>
                        <div className="phase-data tree-mini">
                          <pre>{result.ast?.split('\n').slice(0, 6).join('\n') || '(tree preview)'}...</pre>
                        </div>
                        <div className="analysis-box">
                           <strong>Phase Analysis:</strong>
                           <ul>
                             <li>Validated code structure against C-like grammar rules.</li>
                             <li>Constructed a hierarchical <strong>Abstract Syntax Tree</strong> representing logical flow.</li>
                             <li>Operator precedence (like * before +) was resolved during this phase.</li>
                           </ul>
                        </div>
                        <div className="phase-io">Output: Hierarchical AST</div>
                      </div>
                    </div>

                    <div className="roadmap-arrow">↓</div>

                    {/* Phase 3: Semantic */}
                    <div className="roadmap-phase">
                      <div className="phase-marker">3</div>
                      <div className="phase-content">
                        <h4>Semantic Analysis & Symbol Table</h4>
                        <p className="phase-desc">Checks for logic errors and maintains identifier metadata.</p>
                        <div className="phase-data table-mini">
                           <table>
                             <thead><tr><th>Identifier</th><th>Type</th><th>Value</th></tr></thead>
                             <tbody>
                               {result.symbol_table?.slice(0, 3).map((s, i) => (
                                 <tr key={i}><td>{s.name}</td><td>{s.type}</td><td>{s.value ?? '—'}</td></tr>
                               ))}
                               {(!result.symbol_table || result.symbol_table.length === 0) && <tr><td colSpan="3">No variables found</td></tr>}
                             </tbody>
                           </table>
                        </div>
                        <div className="analysis-box">
                           <strong>Phase Analysis:</strong>
                           <ul>
                             <li>Updated Symbol Table with <strong>{result.summary?.variable_count}</strong> unique identifiers.</li>
                             <li>Verified that all variables were declared before being used in expressions.</li>
                             <li>Confirmed data type compatibility for all operations.</li>
                           </ul>
                        </div>
                        <div className="phase-io">Output: Annotated AST + Symbol Table</div>
                      </div>
                    </div>

                    <div className="roadmap-arrow">↓</div>

                    {/* Phase 4: ICG */}
                    <div className="roadmap-phase">
                      <div className="phase-marker">4</div>
                      <div className="phase-content">
                        <h4>Intermediate Code Generation (ICG)</h4>
                        <p className="phase-desc">Translates AST into <strong>Three Address Code (TAC)</strong> for an abstract machine.</p>
                        <div className="phase-data tac-mini">
                          <pre>{result.tac_before?.slice(0, 4)?.map(t => {
                             if (typeof t === 'string') return t;
                             return t.has_op && t.has_arg2 ? `${t.result} = ${t.arg1} ${t.op} ${t.arg2}` :
                                    t.has_op ? `${t.result} = ${t.op}${t.arg1}` : `${t.result} = ${t.arg1}`;
                          })?.join('\n') || '(icg)'}</pre>
                        </div>
                        <div className="analysis-box">
                           <strong>Phase Analysis:</strong>
                           <ul>
                             <li>Converted complex expressions into a sequence of simple <strong>3-address</strong> instructions.</li>
                             <li>Generated <strong>{result.summary?.tac_before_count}</strong> initial intermediate instructions.</li>
                             <li>Used temporary variables (t1, t2...) to store partial results.</li>
                           </ul>
                        </div>
                        <div className="phase-io">Output: Unoptimized TAC</div>
                      </div>
                    </div>

                    <div className="roadmap-arrow">↓</div>

                    {/* Phase 5: Optimization */}
                    <div className="roadmap-phase highlight">
                      <div className="phase-marker">5</div>
                      <div className="phase-content">
                        <h4>Code Optimization</h4>
                        <p className="phase-desc">Performs <strong>Constant Folding</strong> to reduce instruction count.</p>
                        <div className="phase-data tac-mini success-mini">
                           <pre>{result.tac?.slice(0, 4)?.map(t => {
                             if (typeof t === 'string') return t;
                             return t.has_op && t.has_arg2 ? `${t.result} = ${t.arg1} ${t.op} ${t.arg2}` :
                                    t.has_op ? `${t.result} = ${t.op}${t.arg1}` : `${t.result} = ${t.arg1}`;
                           })?.join('\n') || '(optimized)'}</pre>
                        </div>
                        <div className="analysis-box">
                           <strong>Phase Analysis:</strong>
                           <ul>
                             <li>Applied <strong>Constant Folding</strong> to pre-calculate values at compile-time.</li>
                             <li>Reduced instructions from <strong>{result.summary?.tac_before_count} to {result.summary?.tac_count}</strong>.</li>
                             <li>Optimization Ratio: <strong>{result.summary?.tac_before_count > 0 ? ((1 - (result.summary?.tac_count / result.summary?.tac_before_count)) * 100).toFixed(1) : 0}%</strong> reduction.</li>
                           </ul>
                        </div>
                        <div className="phase-io">Output: Optimized Intermediate Code</div>
                      </div>
                    </div>

                    <div className="roadmap-arrow">↓</div>

                    {/* Phase 6: Final Target Code */}
                    <div className="roadmap-phase final-target">
                      <div className="phase-marker">6</div>
                      <div className="phase-content">
                        <h4>Final Target Code Generation</h4>
                        <p className="phase-desc">Mapping optimized instructions to <strong>Assembly/Machine Code</strong> using CPU Registers.</p>
                        <div className="phase-data assembly-view">
                           <pre>
{`--- Final Target Code Generation ---`}
{"\n"}
{result.tac?.map((t, i) => {
  const r = `R${(i % 8) + 1}`;
  if (typeof t === 'string') {
    const parts = t.split('=');
    return `LOAD  ${r}, ${parts[1]?.trim() || '0'}\nSTORE ${parts[0]?.trim() || 'var'}, ${r}`;
  }
  if (!t.has_op) return `LOAD  ${r}, ${t.arg1}\nSTORE ${t.result}, ${r}`;
  const opMap = { '+': 'ADD ', '-': 'SUB ', '*': 'MUL ', '/': 'DIV ' };
  return `LOAD  ${r}, ${t.arg1}\nLOAD  R10, ${t.arg2}\n${opMap[t.op] || 'MOV '}${r}, R10\nSTORE ${t.result}, ${r}`;
}).join('\n\n')}
                           </pre>
                        </div>
                        <div className="analysis-box">
                           <strong>Phase Analysis:</strong>
                           <ul>
                             <li>Selected registers (R1-R8) for efficient data movement.</li>
                             <li>Mapped abstract operators to specific CPU instructions (ADD, SUB, MUL).</li>
                             <li>Final code is now ready for loading into memory and execution.</li>
                           </ul>
                        </div>
                        <div className="phase-io">Output: Machine Executable Code</div>
                      </div>
                    </div>
                  </div>
                </div>
              )}

              {activeTab === 'summary' && (
                <div>
                  <h3>Compilation Summary</h3>
                  <div className="summary-cards">
                    <div className="card">
                      <div className="card-value">{result.summary?.token_count ?? 0}</div>
                      <div className="card-label">Tokens</div>
                    </div>
                    <div className="card">
                      <div className="card-value">{result.summary?.variable_count ?? 0}</div>
                      <div className="card-label">Variables</div>
                    </div>
                    <div className="card">
                      <div className="card-value">{result.summary?.tac_before_count ?? 0} → {result.summary?.tac_count ?? 0}</div>
                      <div className="card-label">TAC (Before → After)</div>
                    </div>
                    <div className={`card ${result.success ? 'success' : 'fail'}`}>
                      <div className="card-value">{result.success ? '✓' : '✗'}</div>
                      <div className="card-label">{result.success ? 'Success' : 'Failed'}</div>
                    </div>
                  </div>

                  {(result.lexical_errors?.length > 0 || result.syntax_errors?.length > 0 || result.semantic_errors?.length > 0) && (
                    <div className="alert error" style={{ marginTop: '1.5rem' }}>
                      <strong>All Errors (with Line & Column):</strong>
                      <ul>
                        {result.lexical_errors?.map((e, i) => <li key={`l${i}`}>{e}</li>)}
                        {result.syntax_errors?.map((e, i) => <li key={`s${i}`}>{e}</li>)}
                        {result.semantic_errors?.map((e, i) => <li key={`m${i}`}>{e}</li>)}
                      </ul>
                    </div>
                  )}

                  {result.success && (
                    <div className="alert success-box" style={{ marginTop: '1rem' }}>
                      ✅ Compilation successful! All phases completed. Notice how Constant Folding reduced the number of TAC instructions.
                    </div>
                  )}
                </div>
              )}
            </div>
          </div>
        )}

        {!result && !error && (
          <div className="placeholder">
            <p>Write code and click <strong>Compile</strong> to learn every compiler phase visually.</p>
            <p className="hint">Lexical → Syntax (AST) → Semantic + TAC + Constant Folding (Before vs After)</p>
          </div>
        )}
      </main>
    </div>
  )
}

export default App
