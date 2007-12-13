/*
 * (c) Fachhochschule Potsdam
 */
package org.fritzing.fritzing.diagram.edit.parts;

import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.RectangleFigure;
import org.eclipse.draw2d.StackLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.Request;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.editpolicies.LayoutEditPolicy;
import org.eclipse.gef.editpolicies.NonResizableEditPolicy;
import org.eclipse.gef.requests.CreateRequest;
import org.eclipse.gmf.runtime.diagram.ui.editparts.IBorderItemEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.IGraphicalEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.IRotatableEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.BorderItemSelectionEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CreationEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.DragDropEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles;
import org.eclipse.gmf.runtime.diagram.ui.figures.BorderItemLocator;
import org.eclipse.gmf.runtime.draw2d.ui.figures.ConstrainedToolbarLayout;
import org.eclipse.gmf.runtime.draw2d.ui.figures.WrapLabel;
import org.eclipse.gmf.runtime.gef.ui.figures.DefaultSizeNodeFigure;
import org.eclipse.gmf.runtime.gef.ui.figures.NodeFigure;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.gmf.runtime.notation.impl.NodeImpl;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Display;
import org.fritzing.fritzing.Terminal;
import org.fritzing.fritzing.diagram.edit.policies.ArduinoCanonicalEditPolicy;
import org.fritzing.fritzing.diagram.edit.policies.ArduinoItemSemanticEditPolicy;
import org.fritzing.fritzing.diagram.part.FritzingVisualIDRegistry;
import org.eclipse.swt.graphics.Color;
import org.eclipse.gmf.runtime.diagram.ui.figures.IBorderItemLocator;
import java.util.List;
import org.eclipse.draw2d.geometry.Rectangle;

/**
 * @generated NOT
 */
public class ArduinoEditPart extends PartEditPart implements IRotatableEditPart {

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 2001;

	/**
	 * @generated
	 */
	protected IFigure contentPane;

	/**
	 * @generated
	 */
	protected IFigure primaryShape;

	/**
	 * @generated
	 */
	public ArduinoEditPart(View view) {
		super(view);
	}

	/**
	 * @generated
	 */
	protected void createDefaultEditPolicies() {
		installEditPolicy(EditPolicyRoles.CREATION_ROLE,
				new CreationEditPolicy());

		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new ArduinoItemSemanticEditPolicy());
		installEditPolicy(EditPolicyRoles.DRAG_DROP_ROLE,
				new DragDropEditPolicy());
		installEditPolicy(EditPolicyRoles.CANONICAL_ROLE,
				new ArduinoCanonicalEditPolicy());
		installEditPolicy(EditPolicy.LAYOUT_ROLE, createLayoutEditPolicy());
		// XXX need an SCR to runtime to have another abstract superclass that would let children add reasonable editpolicies
		// removeEditPolicy(org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles.CONNECTION_HANDLES_ROLE);
	}

	/**
	 * @generated NOT
	 */
	protected LayoutEditPolicy createLayoutEditPolicy() {
		return super.createLayoutEditPolicy();
	}

	/**
	 * @generated
	 */
	protected IFigure createNodeShape() {
		ArduinoFigure figure = new ArduinoFigure();
		return primaryShape = figure;
	}

	/**
	 * @generated
	 */
	public ArduinoFigure getPrimaryShape() {
		return (ArduinoFigure) primaryShape;
	}

	/**
	 * @generated NOT
	 */
	protected boolean addFixedChild(EditPart childEditPart) {
		if (childEditPart instanceof ArduinoNameEditPart) {
			((ArduinoNameEditPart) childEditPart).setLabel(getPrimaryShape()
					.getFigureArduinoNameFigure());
			return true;
		}
		if (childEditPart instanceof Terminal2EditPart) {
			ArduinoBorderItemLocator.TerminalPosition terminalPosition = ArduinoBorderItemLocator.TerminalPosition.UNKNOWN;
			Object model = childEditPart.getModel();
			
			if (model instanceof NodeImpl) {
				EObject eobject = ((NodeImpl) model).getElement();
				if (eobject instanceof Terminal) {
					String name = ((Terminal) eobject).getName();
					terminalPosition = ArduinoBorderItemLocator.parseTerminalName(name);
				}
			}
			
			IBorderItemLocator locator = null;
			if (terminalPosition == ArduinoBorderItemLocator.TerminalPosition.UNKNOWN) {
				locator = new BorderItemLocator(getMainFigure(), PositionConstants.NONE);
			}
			else {
				locator = new ArduinoBorderItemLocator(getMainFigure(), this, terminalPosition);
			}
			getBorderedFigure().getBorderItemContainer().add(
					((Terminal2EditPart) childEditPart).getFigure(), locator);
			
			return true;
		}
		return false;
	}

	/**
	 * @generated NOT
	 */
	public int getTerminalNamePosition(Terminal2EditPart terminal2, int defaultPosition) {
		
		terminal2.setTextColor(new Color(null, 255, 255, 255));	
		
		String name = null;
		Object model = terminal2.getModel();
		if (model instanceof NodeImpl) {
			EObject eobject = ((NodeImpl) model).getElement();
			if (eobject instanceof Terminal) {
				name = ((Terminal) eobject).getName();					
			}
		}
		
		if (name == null) return defaultPosition;
				
		ArduinoBorderItemLocator.TerminalPosition terminalPosition = ArduinoBorderItemLocator.parseTerminalName(name);
		if (terminalPosition == ArduinoBorderItemLocator.TerminalPosition.UNKNOWN) return defaultPosition;

		if (terminalPosition.getSide() == PositionConstants.NORTH) {
			return PositionConstants.SOUTH;
		}
		else if (terminalPosition.getSide() == PositionConstants.SOUTH) {
			return PositionConstants.NORTH;
		}
		
		
		return defaultPosition;
		
	}

	/**
	 * @generated
	 */
	protected boolean removeFixedChild(EditPart childEditPart) {

		if (childEditPart instanceof Terminal2EditPart) {
			getBorderedFigure().getBorderItemContainer().remove(
					((Terminal2EditPart) childEditPart).getFigure());
			return true;
		}
		return false;
	}

	/**
	 * @generated
	 */
	protected void addChildVisual(EditPart childEditPart, int index) {
		if (addFixedChild(childEditPart)) {
			return;
		}
		super.addChildVisual(childEditPart, -1);
	}

	/**
	 * @generated
	 */
	protected void removeChildVisual(EditPart childEditPart) {
		if (removeFixedChild(childEditPart)) {
			return;
		}
		super.removeChildVisual(childEditPart);
	}

	/**
	 * @generated
	 */
	protected IFigure getContentPaneFor(IGraphicalEditPart editPart) {

		if (editPart instanceof Terminal2EditPart) {
			return getBorderedFigure().getBorderItemContainer();
		}
		return super.getContentPaneFor(editPart);
	}

	/**
	 * @generated
	 */
	protected NodeFigure createNodePlate() {
		DefaultSizeNodeFigure result = new DefaultSizeNodeFigure(getMapMode()
				.DPtoLP(350), getMapMode().DPtoLP(250));
		return result;
	}

	/**
	 * @generated NOT
	 */
	public EditPolicy getPrimaryDragEditPolicy() {
		return super.getPrimaryDragEditPolicy();
	}

	/**
	 * Creates figure for this edit part.
	 * 
	 * Body of this method does not depend on settings in generation model
	 * so you may safely remove <i>generated</i> tag and modify it.
	 * 
	 * @generated
	 */
	protected NodeFigure createMainFigure() {
		NodeFigure figure = createNodePlate();
		figure.setLayoutManager(new StackLayout());
		IFigure shape = createNodeShape();
		figure.add(shape);
		contentPane = setupContentPane(shape);
		
		return figure;
	}

	/**
	 * Default implementation treats passed figure as content pane.
	 * Respects layout one may have set for generated figure.
	 * @param nodeShape instance of generated figure class
	 * @generated
	 */
	protected IFigure setupContentPane(IFigure nodeShape) {
		if (nodeShape.getLayoutManager() == null) {
			ConstrainedToolbarLayout layout = new ConstrainedToolbarLayout();
			layout.setSpacing(getMapMode().DPtoLP(5));
			nodeShape.setLayoutManager(layout);
		}
		return nodeShape; // use nodeShape itself as contentPane
	}

	/**
	 * @generated
	 */
	public IFigure getContentPane() {
		if (contentPane != null) {
			return contentPane;
		}
		return super.getContentPane();
	}

	/**
	 * @generated
	 */
	public EditPart getPrimaryChildEditPart() {
		return getChildBySemanticHint(FritzingVisualIDRegistry
				.getType(ArduinoNameEditPart.VISUAL_ID));
	}

	/**
	 * @generated
	 */
	public class ArduinoFigure extends RectangleFigure {

		/**
		 * @generated
		 */
		private WrapLabel fFigureArduinoNameFigure;

		/**
		 * @generated
		 */
		public ArduinoFigure() {
			this.setForegroundColor(THIS_FORE);
			this.setBackgroundColor(THIS_BACK);
			this.setPreferredSize(new Dimension(getMapMode().DPtoLP(350),
					getMapMode().DPtoLP(250)));
			createContents();
		}

		/**
		 * @generated
		 */
		private void createContents() {
			setLayoutManager(new StackLayout() {
			    public void layout(IFigure figure) {
			        Rectangle r = figure.getClientArea();
					List children = figure.getChildren();
					IFigure child;
					Dimension d;
					for (int i = 0; i < children.size(); i++) {
						child = (IFigure)children.get(i);
						d = child.getPreferredSize(r.width, r.height);
						d.width = Math.min(d.width, r.width);
						d.height = Math.min(d.height, r.height);
						Rectangle childRect = new Rectangle(
						r.x + (r.width - d.width)/2,
						r.y + (r.height - d.height)/2,
						d.width,
						d.height);
						child.setBounds(childRect);
					}
				}
			});
			fFigureArduinoNameFigure = new WrapLabel();
			fFigureArduinoNameFigure.setText("..");

			fFigureArduinoNameFigure.setFont(FFIGUREARDUINONAMEFIGURE_FONT);

			this.add(fFigureArduinoNameFigure);

		}

		/**
		 * @generated
		 */
		private boolean myUseLocalCoordinates = false;

		/**
		 * @generated
		 */
		protected boolean useLocalCoordinates() {
			return myUseLocalCoordinates;
		}

		/**
		 * @generated
		 */
		protected void setUseLocalCoordinates(boolean useLocalCoordinates) {
			myUseLocalCoordinates = useLocalCoordinates;
		}

		/**
		 * @generated
		 */
		public WrapLabel getFigureArduinoNameFigure() {
			return fFigureArduinoNameFigure;
		}

	}

	/**
	 * @generated
	 */
	static final Color THIS_FORE = new Color(null, 0, 0, 0);

	/**
	 * @generated
	 */
	static final Color THIS_BACK = new Color(null, 39, 128, 157);

	/**
	 * @generated
	 */
	static final Font FFIGUREARDUINONAMEFIGURE_FONT = new Font(Display
			.getCurrent(),
			Display.getDefault().getSystemFont().getFontData()[0].getName(), 8,
			SWT.NORMAL);

}
